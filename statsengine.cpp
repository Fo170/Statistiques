#include "statsengine.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cfloat>

StatsEngine::StatsEngine()
{
    m_rg.mode = 0;
    m_rg.xmin = m_rg.ymin = LDBL_MAX;
    m_rg.xmax = m_rg.ymax = -LDBL_MAX;
    m_rg.tx = m_rg.ty = 0;
    m_rg.n = 0;
    m_rg.a = m_rg.b = m_rg.r = m_rg.rcrit = m_rg.cov = 0;
    m_rg.sx = m_rg.sy = m_rg.sxy = m_rg.sx2 = m_rg.sy2 = 0;
}

void StatsEngine::setMode(int md)
{
    m_rg.mode = md;
}

int StatsEngine::mode() const
{
    return m_rg.mode;
}

void StatsEngine::regTpl(const double* xd, const double* yd, ulong nd)
{
    ulong i;
    Ldbl x, y, Rnsx2_psx, Rnsxy_sxsy;

    m_rg.xmin = m_rg.ymin = LDBL_MAX;
    m_rg.xmax = m_rg.ymax = -LDBL_MAX;
    m_rg.n = (Ldbl)nd;

    for (i = 0; i < nd; i++) {
        x = _L(xd + i);
        y = _L(yd + i);
        if (x < m_rg.xmin) m_rg.xmin = x;
        if (y < m_rg.ymin) m_rg.ymin = y;
        if (x > m_rg.xmax) m_rg.xmax = x;
        if (y > m_rg.ymax) m_rg.ymax = y;
    }

    m_rg.tx = 0.0; m_rg.ty = 0.0;
    if (m_rg.mode == 1 && m_rg.xmin <= 0.0) m_rg.tx = 1.0 - m_rg.xmin;
    if (m_rg.mode == 2 && m_rg.ymin <= 0.0) m_rg.ty = 1.0 - m_rg.ymin;
    // mode 3: no shift (tx=0, ty=0) -- points with x<=0 or y<=0
    // contribute no information for a,b in log-linearized space

    m_rg.sx = m_rg.sy = m_rg.sxy = m_rg.sx2 = m_rg.sy2 = 0.0;
    Ldbl nn = 0;
    for (i = 0; i < nd; i++) {
        x = _L(xd + i) + m_rg.tx;
        y = _L(yd + i) + m_rg.ty;
        if ((m_rg.mode == 1) || (m_rg.mode == 3)) {
            if (x <= 0.0) continue;
            x = logl(x);
        }
        if ((m_rg.mode == 2) || (m_rg.mode == 3)) {
            if (y <= 0.0) continue;
            y = logl(y);
        }
        m_rg.sx  += x;
        m_rg.sy  += y;
        m_rg.sxy += x * y;
        m_rg.sx2 += pw2(x);
        m_rg.sy2 += pw2(y);
        nn += 1.0;
    }

    if (nn < 2.0) return;

    Rnsxy_sxsy = nn * m_rg.sxy - m_rg.sx * m_rg.sy;
    Rnsx2_psx  = nn * m_rg.sx2 - pw2(m_rg.sx);

    m_rg.b = Rnsxy_sxsy / Rnsx2_psx;
    m_rg.a = (m_rg.sy - m_rg.b * m_rg.sx) / nn;

    if ((m_rg.mode == 2) || (m_rg.mode == 3)) m_rg.a = expl(m_rg.a);

    // r, r², cov on ALL points in ORIGINAL space
    Ldbl sy_tot = 0, sy2 = 0, y_avg = 0, x_avg = 0;
    Ldbl r_num = 0, r_den_x = 0, r_den_y = 0;
    for (i = 0; i < nd; i++) {
        x_avg += _L(xd + i);
        y_avg += _L(yd + i);
    }
    x_avg /= m_rg.n;
    y_avg /= m_rg.n;

    for (i = 0; i < nd; i++) {
        Ldbl xi = _L(xd + i);
        Ldbl yi = _L(yd + i);
        Ldbl yp = regFY(xi);
        Ldbl dy = yi - y_avg;
        Ldbl dr = yi - yp;
        Ldbl dx = xi - x_avg;
        Ldbl dy_pred = yp - y_avg;
        sy_tot += dy * dy;
        sy2    += dr * dr;
        r_num   += dx * dy_pred;
        r_den_x += dx * dx;
        r_den_y += dy_pred * dy_pred;
    }

    m_rg.rcrit = (sy_tot > 0.0) ? 1.0 - sy2 / sy_tot : 0.0;
    m_rg.r = (r_den_x > 0 && r_den_y > 0)
             ? r_num / sqrtl(r_den_x * r_den_y) : 0.0;
    m_rg.cov = sy2 / (m_rg.n - 2.0); // residual variance (MSE)
}

void StatsEngine::regNls(const double* xd, const double* yd, ulong nd)
{
    // Gauss-Newton pour y = a * x^b
    ulong i;
    m_rg.mode = 4;
    m_rg.tx = 0.0; m_rg.ty = 0.0;

    m_rg.xmin = m_rg.ymin = LDBL_MAX;
    m_rg.xmax = m_rg.ymax = -LDBL_MAX;
    m_rg.n = (Ldbl)nd;
    for (i = 0; i < nd; i++) {
        Ldbl x = _L(xd + i), y = _L(yd + i);
        if (x < m_rg.xmin) m_rg.xmin = x;
        if (y < m_rg.ymin) m_rg.ymin = y;
        if (x > m_rg.xmax) m_rg.xmax = x;
        if (y > m_rg.ymax) m_rg.ymax = y;
    }

    // Initialisation par log-linearisation sur points >0
    Ldbl a = 1.0, b = 1.0;
    {
        Ldbl sx = 0, sy = 0, sxy = 0, sx2 = 0, nn = 0;
        for (i = 0; i < nd; i++) {
            Ldbl x = _L(xd + i), y = _L(yd + i);
            if (x > 0.0 && y > 0.0) {
                x = logl(x); y = logl(y);
                sx += x; sy += y; sxy += x * y; sx2 += x * x;
                nn += 1.0;
            }
        }
        if (nn >= 2.0) {
            Ldbl Rnsxy_sxsy = nn * sxy - sx * sy;
            Ldbl Rnsx2_psx  = nn * sx2 - sx * sx;
            if (fabsl(Rnsx2_psx) > 1e-30) {
                b = Rnsxy_sxsy / Rnsx2_psx;
                a = expl((sy - b * sx) / nn);
            }
        }
    }

    // Iterations Gauss-Newton
    Ldbl prev_obj = 1e100;
    for (int iter = 0; iter < 30; iter++) {
        Ldbl JtJ[2][2] = {{0,0},{0,0}};
        Ldbl Jtr[2] = {0,0};
        Ldbl obj = 0;

        for (i = 0; i < nd; i++) {
            Ldbl xi = _L(xd + i), yi = _L(yd + i);
            Ldbl J1, J2, r;
            if (xi == 0.0) {
                r = yi;
                J1 = 0.0; J2 = 0.0;
            } else {
                Ldbl xb = powl(xi, b);
                r = yi - a * xb;
                J1 = -xb;
                J2 = -a * xb * logl(xi);
            }
            obj += r * r;
            JtJ[0][0] += J1 * J1; JtJ[0][1] += J1 * J2;
            JtJ[1][0] += J1 * J2; JtJ[1][1] += J2 * J2;
            Jtr[0] -= J1 * r; Jtr[1] -= J2 * r;
        }

        if (iter > 0 && fabsl(prev_obj - obj) < 1e-12 * (1.0 + obj))
            break;
        prev_obj = obj;

        Ldbl det = JtJ[0][0] * JtJ[1][1] - JtJ[0][1] * JtJ[1][0];
        if (fabsl(det) < 1e-30) break;

        Ldbl da = (JtJ[1][1] * Jtr[0] - JtJ[0][1] * Jtr[1]) / det;
        Ldbl db = (JtJ[0][0] * Jtr[1] - JtJ[1][0] * Jtr[0]) / det;

        // Recherche lineaire : essai de pas 1, 1/2, 1/4...
        Ldbl best_a = a, best_b = b, best_obj = prev_obj;
        Ldbl step = 1.0;
        for (int ls = 0; ls < 15; ls++) {
            Ldbl ta = a + step * da;
            Ldbl tb = b + step * db;
            if (ta > 0.0) {
                Ldbl new_obj = 0;
                for (i = 0; i < nd; i++) {
                    Ldbl xi = _L(xd + i), yi = _L(yd + i);
                    if (xi == 0.0) new_obj += yi * yi;
                    else { Ldbl res = yi - ta * powl(xi, tb); new_obj += res * res; }
                }
                if (new_obj < best_obj) { best_obj = new_obj; best_a = ta; best_b = tb; }
            }
            step *= 0.5;
        }
        if (best_obj < prev_obj) { a = best_a; b = best_b; }
        else { a += da; b += db; if (a <= 0.0) a = 1e-10; }
    }

    m_rg.a = a; m_rg.b = b;

    // r, r², cov dans l'espace original sur tous les points
    Ldbl sy_tot = 0, sy2 = 0, y_avg = 0, x_avg = 0;
    Ldbl r_num = 0, r_den_x = 0, r_den_y = 0;
    for (i = 0; i < nd; i++) {
        x_avg += _L(xd + i);
        y_avg += _L(yd + i);
    }
    x_avg /= m_rg.n;
    y_avg /= m_rg.n;

    for (i = 0; i < nd; i++) {
        Ldbl xi = _L(xd + i), yi = _L(yd + i);
        Ldbl yp = regFY(xi);
        Ldbl dy = yi - y_avg;
        Ldbl dr = yi - yp;
        Ldbl dx = xi - x_avg;
        Ldbl dy_pred = yp - y_avg;
        sy_tot += dy * dy;
        sy2    += dr * dr;
        r_num   += dx * dy_pred;
        r_den_x += dx * dx;
        r_den_y += dy_pred * dy_pred;
    }

    m_rg.rcrit = (sy_tot > 0.0) ? 1.0 - sy2 / sy_tot : 0.0;
    m_rg.r = (r_den_x > 0 && r_den_y > 0)
             ? r_num / sqrtl(r_den_x * r_den_y) : 0.0;
    m_rg.cov = sy2 / (m_rg.n - 2.0);
}

void StatsEngine::compute(const std::vector<double>& xd, const std::vector<double>& yd)
{
    if (m_rg.mode == 4)
        regNls(xd.data(), yd.data(), (ulong)xd.size());
    else
        regTpl(xd.data(), yd.data(), (ulong)xd.size());
}

int StatsEngine::autoMode(const std::vector<double>& xd, const std::vector<double>& yd)
{
    int bestMode = 0;
    Ldbl bestR = 0.0;

    for (int i = 0; i < 5; i++) {
        m_rg.mode = i;
        compute(xd, yd);
        Ldbl rAbs = fabsl(m_rg.rcrit);
        if (rAbs > bestR) {
            bestR = rAbs;
            bestMode = i;
        }
    }

    m_rg.mode = bestMode;
    compute(xd, yd);
    return bestMode;
}

Ldbl StatsEngine::regFY(Ldbl x) const
{
    Ldbl y = 0;
    if (m_rg.mode == 0) y = m_rg.a + m_rg.b * x;
    if (m_rg.mode == 1) y = m_rg.a + m_rg.b * logl(x + m_rg.tx);
    if (m_rg.mode == 2) y = m_rg.a * expl(m_rg.b * x) - m_rg.ty;
    if (m_rg.mode == 3) y = m_rg.a * powl(x + m_rg.tx, m_rg.b) - m_rg.ty;
    if (m_rg.mode == 4) {
        if (x <= 0.0 && m_rg.b > 0.0) y = 0.0;
        else y = m_rg.a * powl(x, m_rg.b);
    }
    return y;
}

Ldbl StatsEngine::regFX(Ldbl y) const
{
    Ldbl x = 0;
    if (m_rg.mode == 0) x = (y - m_rg.a) / m_rg.b;
    if (m_rg.mode == 1) x = expl((y - m_rg.a) / m_rg.b) - m_rg.tx;
    if (m_rg.mode == 2) x = logl((y + m_rg.ty) / m_rg.a) / m_rg.b - m_rg.tx;
    if (m_rg.mode == 3) x = powl(((y + m_rg.ty) / m_rg.a), 1.0 / m_rg.b) - m_rg.tx;
    if (m_rg.mode == 4) {
        if (y <= 0.0 && m_rg.b > 0.0) x = 0.0;
        else x = powl(y / m_rg.a, 1.0 / m_rg.b);
    }
    return x;
}

void StatsEngine::sortData(std::vector<double>& xd, std::vector<double>& yd)
{
    ulong n = (ulong)xd.size();
    for (ulong i = 0; i < n - 1; i++) {
        ulong jmin = i;
        for (ulong j = i + 1; j < n; j++) {
            if (xd[j] < xd[jmin]) jmin = j;
        }
        if (jmin != i) {
            std::swap(xd[i], xd[jmin]);
            std::swap(yd[i], yd[jmin]);
        }
    }

    for (ulong i = 0; i < n - 1; i++) {
        if (xd[i] == xd[i + 1]) {
            ulong k = i + 1;
            while (k < n && xd[k] == xd[i]) k++;
            ulong cnt = k - i;
            if (cnt > 1) {
                std::sort(yd.begin() + i, yd.begin() + i + cnt);
            }
        }
    }
}

void StatsEngine::printResults() const
{
    printf("%s", resultsText().c_str());
}

std::string StatsEngine::resultsText() const
{
    std::ostringstream os;
    os << std::scientific << std::setprecision(6);

    if (m_rg.mode == 0) {
        os << "REGRESSION LINEAIRE (MODE 0)\n";
        os << "y = a + b*x , x = (y - a)/b\n";
    } else if (m_rg.mode == 1) {
        os << "REGRESSION LOGARITHMIQUE (MODE 1)\n";
        os << "y = a + b*ln(x+tx) , x = e^((y-a)/b) - tx\n";
    } else if (m_rg.mode == 2) {
        os << "REGRESSION EXPONENTIELLE (MODE 2)\n";
        os << "y = a*e^(b*x) - ty , x = ln((y+ty)/a)/b - tx\n";
    } else if (m_rg.mode == 3) {
        os << "REGRESSION PUISSANCE (MODE 3)\n";
        os << "y = a*(x+tx)^b - ty , x = ((y+ty)/a)^(1/b) - tx\n";
    } else if (m_rg.mode == 4) {
        os << "REGRESSION PUISSANCE NLS (MODE 4)\n";
        os << "y = a*x^b , x = (y/a)^(1/b)\n";
    }

    os << "A = " << m_rg.a << " , B = " << m_rg.b
       << " , r = " << m_rg.r << "\n";
    os << "coefficient critique = " << m_rg.rcrit
       << " , covariance = " << m_rg.cov << "\n";
    os << "SX = " << m_rg.sx << " , SX^2 = " << m_rg.sx2 << "\n";
    os << "SY = " << m_rg.sy << " , SY^2 = " << m_rg.sy2 << "\n";
    os << "SXY = " << m_rg.sxy << " , N = " << m_rg.n << "\n";
    os << "Tx = " << m_rg.tx << " , Ty = " << m_rg.ty << "\n";
    os << "XMIN = " << m_rg.xmin << " , XMAX = " << m_rg.xmax << "\n";
    os << "YMIN = " << m_rg.ymin << " , YMAX = " << m_rg.ymax << "\n";

    return os.str();
}
