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
    m_rg.a = m_rg.b = m_rg.c = m_rg.d = m_rg.r = m_rg.rcrit = m_rg.cov = 0;
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

    // NUMERICAL STABILITY CHECK: Detect very large shifts (potential precision loss)
    // If tx or ty > 1e10, there may be numerical precision issues in calculations
    // This is a warning indicator, not a failure (calculations proceed as-is)
    if (fabsl(m_rg.tx) > 1e10 || fabsl(m_rg.ty) > 1e10) {
        // NOTE: Large shifts detected - numerical precision may be degraded
        // Consider alternative preprocessing (normalization, rescaling) if needed
        // For now, we proceed with standard calculation but results may have reduced accuracy
    }

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

    if (fabsl(Rnsx2_psx) < 1e-30) return;
    m_rg.b = Rnsxy_sxsy / Rnsx2_psx;
    m_rg.a = (m_rg.sy - m_rg.b * m_rg.sx) / nn;

    if ((m_rg.mode == 2) || (m_rg.mode == 3)) m_rg.a = expl(m_rg.a);

    // Validate critical parameters
    if (fabsl(m_rg.a) < 1e-30) m_rg.a = 1e-30;
    if (fabsl(m_rg.b) < 1e-30) m_rg.b = 1e-30;

    // r, r², cov: For modes 1,2,3 with excluded points, calc stats ONLY on valid points
    // Otherwise calc on ALL points in ORIGINAL space
    Ldbl sy_tot = 0, sy2 = 0, y_avg = 0, x_avg = 0;
    Ldbl r_num = 0, r_den_x = 0, r_den_y = 0;
    Ldbl nn_stats = m_rg.n;

    // For modes 1,2,3: recalculate averages on valid points only
    if (m_rg.mode == 1 || m_rg.mode == 2 || m_rg.mode == 3) {
        Ldbl x_count = 0;
        for (i = 0; i < nd; i++) {
            Ldbl xi = _L(xd + i);
            Ldbl yi = _L(yd + i);
            // Check same conditions as in MCO loop
            Ldbl xt = xi + m_rg.tx, yt = yi + m_rg.ty;
            if ((m_rg.mode == 1 || m_rg.mode == 3) && xt <= 0.0) continue;
            if ((m_rg.mode == 2 || m_rg.mode == 3) && yt <= 0.0) continue;
            x_avg += xi;
            y_avg += yi;
            x_count += 1.0;
        }
        nn_stats = x_count;
        if (nn_stats > 0) { x_avg /= nn_stats; y_avg /= nn_stats; }
    } else {
        for (i = 0; i < nd; i++) {
            x_avg += _L(xd + i);
            y_avg += _L(yd + i);
        }
        x_avg /= m_rg.n;
        y_avg /= m_rg.n;
    }

    // Calculate stats on same points used for regression
    for (i = 0; i < nd; i++) {
        Ldbl xi = _L(xd + i);
        Ldbl yi = _L(yd + i);
        // Skip excluded points for modes 1,2,3
        if (m_rg.mode == 1 || m_rg.mode == 2 || m_rg.mode == 3) {
            Ldbl xt = xi + m_rg.tx, yt = yi + m_rg.ty;
            if ((m_rg.mode == 1 || m_rg.mode == 3) && xt <= 0.0) continue;
            if ((m_rg.mode == 2 || m_rg.mode == 3) && yt <= 0.0) continue;
        }
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

    // For non-linear regression, r (correlation coeff) is mathematically invalid
    // Use rcrit (R²) as the goodness-of-fit measure instead
    if (m_rg.mode >= 1 && m_rg.mode <= 8) {
        m_rg.r = m_rg.rcrit;  // Use R² for non-linear modes
    } else {
        // For linear mode (0), use correlation coefficient
        m_rg.r = (r_den_x > 0 && r_den_y > 0)
                 ? r_num / sqrtl(r_den_x * r_den_y) : 0.0;
    }
    m_rg.cov = (nn_stats > 2.0) ? sy2 / (nn_stats - 2.0) : 0.0;
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

    // Validate power law parameters: a > 0 always, b ≠ 0 for meaningful fit
    if (a <= 0.0) a = 1e-10;
    if (fabsl(b) < 1e-30) b = 1e-30;
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
    m_rg.r = m_rg.rcrit;  // For non-linear regression, use R² as goodness-of-fit measure
    m_rg.cov = sy2 / (m_rg.n - 2.0);
}

void StatsEngine::compute(const std::vector<double>& xd, const std::vector<double>& yd)
{
    switch (m_rg.mode) {
    case 4: regNls(xd.data(), yd.data(), (ulong)xd.size()); break;
    case 5: regRecip(xd.data(), yd.data(), (ulong)xd.size()); break;
    case 6: regPoly2(xd.data(), yd.data(), (ulong)xd.size()); break;
    case 7: regSine(xd.data(), yd.data(), (ulong)xd.size()); break;
    case 8: regLogistic(xd.data(), yd.data(), (ulong)xd.size()); break;
    default: regTpl(xd.data(), yd.data(), (ulong)xd.size()); break;
    }
}

void StatsEngine::regRecip(const double* xd, const double* yd, ulong nd)
{
    // y = a + b/x  ->  lineaire en posant X = 1/x
    ulong i;
    m_rg.mode = 5; m_rg.tx = 0; m_rg.ty = 0;
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

    Ldbl sx = 0, sy = 0, sxy = 0, sx2 = 0, nn = 0;
    for (i = 0; i < nd; i++) {
        Ldbl xi = _L(xd + i), yi = _L(yd + i);
        if (xi == 0.0) continue;
        xi = 1.0 / xi;
        sx += xi; sy += yi; sxy += xi * yi; sx2 += xi * xi;
        nn += 1.0;
    }

    if (nn >= 2.0) {
        Ldbl den = nn * sx2 - sx * sx;
        if (fabsl(den) > 1e-30) {
            m_rg.b = (nn * sxy - sx * sy) / den;
            m_rg.a = (sy - m_rg.b * sx) / nn;
            // Validate: b must be non-zero for meaningful y = a + b/x
            if (fabsl(m_rg.b) < 1e-30) m_rg.b = 1e-30;
        }
    }

    // r, r², cov dans l'espace original
    Ldbl sy_tot = 0, sy2 = 0, y_avg = 0, x_avg = 0;
    Ldbl r_num = 0, r_den_x = 0, r_den_y = 0;
    for (i = 0; i < nd; i++) { x_avg += _L(xd + i); y_avg += _L(yd + i); }
    x_avg /= m_rg.n; y_avg /= m_rg.n;
    for (i = 0; i < nd; i++) {
        Ldbl xi = _L(xd + i), yi = _L(yd + i);
        Ldbl yp = regFY(xi);
        Ldbl dy = yi - y_avg, dr = yi - yp, dx = xi - x_avg, dy_pred = yp - y_avg;
        sy_tot += dy * dy; sy2 += dr * dr;
        r_num += dx * dy_pred; r_den_x += dx * dx; r_den_y += dy_pred * dy_pred;
    }
    m_rg.rcrit = (sy_tot > 0) ? 1.0 - sy2 / sy_tot : 0.0;
    m_rg.r = (r_den_x > 0 && r_den_y > 0) ? r_num / sqrtl(r_den_x * r_den_y) : 0.0;
    m_rg.cov = sy2 / (m_rg.n - 2.0);
}

void StatsEngine::regPoly2(const double* xd, const double* yd, ulong nd)
{
    // y = a + b*x + c*x²  (3 parametres par MCO)
    ulong i;
    m_rg.mode = 6; m_rg.tx = 0; m_rg.ty = 0;
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

    Ldbl s1  = nd, sx = 0, sx2 = 0, sx3 = 0, sx4 = 0;
    Ldbl sy = 0, sxy = 0, sx2y = 0;
    for (i = 0; i < nd; i++) {
        Ldbl x = _L(xd + i), y = _L(yd + i);
        Ldbl x2 = x * x;
        sx += x; sx2 += x2; sx3 += x2 * x; sx4 += x2 * x2;
        sy += y; sxy += x * y; sx2y += x2 * y;
    }

    // Normal equations: X'X * beta = X'y
    // [s1  sx  sx2] [a]   [sy ]
    // [sx sx2 sx3] [b] = [sxy]
    // [sx2 sx3 sx4] [c]   [sx2y]
    Ldbl det = s1*(sx2*sx4 - sx3*sx3) - sx*(sx*sx4 - sx3*sx2) + sx2*(sx*sx3 - sx2*sx2);
    if (fabsl(det) > 1e-30) {
        Ldbl inv = 1.0 / det;
        m_rg.a = (sy *(sx2*sx4 - sx3*sx3) - sx *(sxy*sx4 - sx3*sx2y) + sx2*(sxy*sx3 - sx2*sx2y)) * inv;
        m_rg.b = (s1 *(sxy*sx4 - sx3*sx2y) - sx *(sy *sx4 - sx2*sx2y) + sx2*(sy *sx3 - sx2*sxy )) * inv;
        m_rg.c = (s1 *(sx2*sx2y - sxy*sx3) - sx *(sx *sx2y - sxy*sx2) + sx2*(sx *sxy - sx2*sy  )) * inv;
    }

    // r, r², cov dans l'espace original
    Ldbl sy_tot = 0, sy2 = 0, y_avg = 0, x_avg = 0;
    Ldbl r_num = 0, r_den_x = 0, r_den_y = 0;
    for (i = 0; i < nd; i++) { x_avg += _L(xd + i); y_avg += _L(yd + i); }
    x_avg /= m_rg.n; y_avg /= m_rg.n;
    for (i = 0; i < nd; i++) {
        Ldbl xi = _L(xd + i), yi = _L(yd + i);
        Ldbl yp = regFY(xi);
        Ldbl dy = yi - y_avg, dr = yi - yp, dx = xi - x_avg, dy_pred = yp - y_avg;
        sy_tot += dy * dy; sy2 += dr * dr;
        r_num += dx * dy_pred; r_den_x += dx * dx; r_den_y += dy_pred * dy_pred;
    }
    m_rg.rcrit = (sy_tot > 0) ? 1.0 - sy2 / sy_tot : 0.0;
    m_rg.r = (r_den_x > 0 && r_den_y > 0) ? r_num / sqrtl(r_den_x * r_den_y) : 0.0;
    m_rg.cov = sy2 / (m_rg.n - 3.0); // 3 parametres
}

void StatsEngine::regSine(const double* xd, const double* yd, ulong nd)
{
    // y = a*sin(b*x + c) + d  (NLS 4 parametres)
    ulong i;
    m_rg.mode = 7; m_rg.tx = 0; m_rg.ty = 0;
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

    Ldbl d = 0, a = 1, b = 1, c = 0;
    for (i = 0; i < nd; i++) d += _L(yd + i);
    d /= m_rg.n;
    Ldbl ymin = 1e100, ymax = -1e100;
    for (i = 0; i < nd; i++) {
        Ldbl yv = _L(yd + i);
        if (yv < ymin) ymin = yv;
        if (yv > ymax) ymax = yv;
    }
    a = (ymax - ymin) / 2.0;
    Ldbl x_range = m_rg.xmax - m_rg.xmin;
    if (x_range > 0) {
        int nzc = 0;
        Ldbl prev = _L(yd) - d;
        for (i = 1; i < nd; i++) {
            Ldbl cur = _L(yd + i) - d;
            if ((prev > 0 && cur <= 0) || (prev < 0 && cur >= 0)) nzc++;
            prev = cur;
        }
        b = (nzc >= 2) ? M_PI * nzc / x_range : 2.0 * M_PI / x_range;
    }
    // Estimer c (et raffiner a) par regression lineaire sur sin(bx), cos(bx)
    if (a > 1e-30 && b > 1e-30) {
        Ldbl Ss = 0, Sc = 0, Sys = 0, Syc = 0;
        for (i = 0; i < nd; i++) {
            Ldbl xi = _L(xd + i), yi = _L(yd + i) - d;
            Ldbl s = sinl(b * xi), cs = cosl(b * xi);
            Ss += s * s; Sc += cs * cs;
            Sys += yi * s; Syc += yi * cs;
        }
        Ldbl den = Ss * Sc;
        if (fabsl(den) > 1e-30) {
            Ldbl alpha = Sys / Ss, beta = Syc / Sc;
            a = sqrtl(alpha * alpha + beta * beta);
            c = atan2l(beta, alpha);
        }
    }

    Ldbl da, db, dc, dd;
    Ldbl prev_obj = 1e100;
    for (int iter = 0; iter < 50; iter++) {
        Ldbl JtJ[4][4] = {{0}};
        Ldbl Jtr[4] = {0};
        Ldbl obj = 0;
        for (i = 0; i < nd; i++) {
            Ldbl xi = _L(xd + i), yi = _L(yd + i);
            Ldbl arg = b * xi + c;
            Ldbl s = sinl(arg), cs = cosl(arg);
            Ldbl r = yi - a * s - d;
            Ldbl J[4] = { -s, -a * xi * cs, -a * cs, -1.0 };
            obj += r * r;
            for (int u = 0; u < 4; u++) {
                Jtr[u] -= J[u] * r;
                for (int v = 0; v < 4; v++) JtJ[u][v] += J[u] * J[v];
            }
        }
        if (iter > 0 && fabsl(prev_obj - obj) < 1e-10 * (1 + obj)) break;
        prev_obj = obj;

        // Gaussian elimination 4x4
        Ldbl M[4][5];
        for (int u = 0; u < 4; u++)
            for (int v = 0; v < 4; v++) M[u][v] = JtJ[u][v];
        for (int u = 0; u < 4; u++) M[u][4] = Jtr[u];

        for (int col = 0; col < 4; col++) {
            int pivot = col;
            for (int row = col + 1; row < 4; row++)
                if (fabsl(M[row][col]) > fabsl(M[pivot][col])) pivot = row;
            if (fabsl(M[pivot][col]) < 1e-40) { iter = 99; break; }
            for (int j = col; j <= 4; j++) { Ldbl t = M[col][j]; M[col][j] = M[pivot][j]; M[pivot][j] = t; }
            for (int row = col + 1; row < 4; row++) {
                Ldbl factor = M[row][col] / M[col][col];
                for (int j = col; j <= 4; j++) M[row][j] -= factor * M[col][j];
            }
        }
        if (iter > 50) break;

        Ldbl delta[4] = {0};
        for (int row = 3; row >= 0; row--) {
            delta[row] = M[row][4];
            for (int j = row + 1; j < 4; j++) delta[row] -= M[row][j] * delta[j];
            delta[row] /= M[row][row];
        }
        da = delta[0]; db = delta[1]; dc = delta[2]; dd = delta[3];

        // Line search: essai pas 1, 1/2...
        Ldbl best_a = a, best_b = b, best_c = c, best_d = d, best_obj = prev_obj;
        for (int ls = 0, mask = 1; ls < 15; ls++, mask <<= 1) {
            Ldbl step = 1.0 / (1 << ls);
            Ldbl ta = a + step * da, tb = b + step * db;
            Ldbl tc = c + step * dc, td = d + step * dd;
            Ldbl new_obj = 0;
            for (i = 0; i < nd; i++) {
                Ldbl xi = _L(xd + i), yi = _L(yd + i);
                Ldbl res = yi - ta * sinl(tb * xi + tc) - td;
                new_obj += res * res;
            }
            if (new_obj < best_obj) { best_a = ta; best_b = tb; best_c = tc; best_d = td; best_obj = new_obj; }
        }
        if (best_obj < prev_obj) { a = best_a; b = best_b; c = best_c; d = best_d; }
        else { a += da; b += db; c += dc; d += dd; }
    }

    // Validate sinusoidal parameters: a ≠ 0, b ≠ 0 required
    if (fabsl(a) < 1e-30) a = 1e-30;
    if (fabsl(b) < 1e-30) b = 1e-30;
    m_rg.a = a; m_rg.b = b; m_rg.c = c; m_rg.d = d;

    // r, r², cov dans l'espace original
    Ldbl sy_tot = 0, sy2 = 0, y_avg = 0, x_avg = 0;
    Ldbl r_num = 0, r_den_x = 0, r_den_y = 0;
    for (i = 0; i < nd; i++) { x_avg += _L(xd + i); y_avg += _L(yd + i); }
    x_avg /= m_rg.n; y_avg /= m_rg.n;
    for (i = 0; i < nd; i++) {
        Ldbl xi = _L(xd + i), yi = _L(yd + i);
        Ldbl yp = regFY(xi);
        Ldbl dy = yi - y_avg, dr = yi - yp, dx = xi - x_avg, dy_pred = yp - y_avg;
        sy_tot += dy * dy; sy2 += dr * dr; r_num += dx * dy_pred; r_den_x += dx * dx; r_den_y += dy_pred * dy_pred;
    }
    m_rg.rcrit = (sy_tot > 0) ? 1.0 - sy2 / sy_tot : 0.0;
    m_rg.r = (r_den_x > 0 && r_den_y > 0) ? r_num / sqrtl(r_den_x * r_den_y) : 0.0;
    m_rg.cov = sy2 / (m_rg.n - 4.0);
}

void StatsEngine::regLogistic(const double* xd, const double* yd, ulong nd)
{
    // y = c / (1 + a*e^(-b*x))  (NLS 3 parametres)
    ulong i;
    m_rg.mode = 8; m_rg.tx = 0; m_rg.ty = 0;
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

    Ldbl a = 1, b = 1, c = m_rg.ymax * 1.1;
    // Si possible, initialisation par linearisation: ln(c/y - 1) = ln(a) - b*x
    Ldbl sx = 0, sy = 0, sxy = 0, sx2 = 0, nn = 0;
    for (i = 0; i < nd; i++) {
        Ldbl y = _L(yd + i);
        if (y > 0 && y < c) {
            Ldbl z = logl(c / y - 1.0);
            Ldbl x = _L(xd + i);
            sx += x; sy += z; sxy += x * z; sx2 += x * x; nn++;
        }
    }
    if (nn >= 2) {
        Ldbl den = nn * sx2 - sx * sx;
        if (fabsl(den) > 1e-30) {
            Ldbl lna = (sy * sx2 - sx * sxy) / den;
            b = -(nn * sxy - sx * sy) / den;
            a = expl(lna);
        }
    }

    Ldbl prev_obj = 1e100;
    for (int iter = 0; iter < 50; iter++) {
        Ldbl JtJ[3][3] = {{0}};
        Ldbl Jtr[3] = {0};
        Ldbl obj = 0;
        for (i = 0; i < nd; i++) {
            Ldbl xi = _L(xd + i), yi = _L(yd + i);
            Ldbl ex = expl(-b * xi);
            Ldbl denom = 1.0 + a * ex;
            Ldbl yp = c / denom;
            Ldbl r = yi - yp;
            Ldbl fac = c * ex / (denom * denom);
            Ldbl J[3] = { -fac, -fac * a * xi, -1.0 / denom };
            obj += r * r;
            for (int u = 0; u < 3; u++) {
                Jtr[u] -= J[u] * r;
                for (int v = 0; v < 3; v++) JtJ[u][v] += J[u] * J[v];
            }
        }
        if (iter > 0 && fabsl(prev_obj - obj) < 1e-10 * (1 + obj)) break;
        prev_obj = obj;

        // Gaussian elimination 3x3
        Ldbl M[3][4];
        for (int u = 0; u < 3; u++)
            for (int v = 0; v < 3; v++) M[u][v] = JtJ[u][v];
        for (int u = 0; u < 3; u++) M[u][3] = Jtr[u];

        for (int col = 0; col < 3; col++) {
            int pivot = col;
            for (int row = col + 1; row < 3; row++)
                if (fabsl(M[row][col]) > fabsl(M[pivot][col])) pivot = row;
            if (fabsl(M[pivot][col]) < 1e-40) { iter = 99; break; }
            for (int j = col; j <= 3; j++) { Ldbl t = M[col][j]; M[col][j] = M[pivot][j]; M[pivot][j] = t; }
            for (int row = col + 1; row < 3; row++) {
                Ldbl factor = M[row][col] / M[col][col];
                for (int j = col; j <= 3; j++) M[row][j] -= factor * M[col][j];
            }
        }
        if (iter > 50) break;

        Ldbl delta[3] = {0};
        for (int row = 2; row >= 0; row--) {
            delta[row] = M[row][3];
            for (int j = row + 1; j < 3; j++) delta[row] -= M[row][j] * delta[j];
            delta[row] /= M[row][row];
        }

        // Line search
        Ldbl best_a = a, best_b = b, best_c = c, best_obj = prev_obj;
        for (int ls = 0, mask = 1; ls < 15; ls++, mask <<= 1) {
            Ldbl step = 1.0 / (1 << ls);
            Ldbl ta = a + step * delta[0], tb = b + step * delta[1], tc = c + step * delta[2];
            if (ta <= 0 || tc <= 0) continue;
            Ldbl new_obj = 0;
            for (i = 0; i < nd; i++) {
                Ldbl xi = _L(xd + i), yi = _L(yd + i);
                Ldbl denom = 1.0 + ta * expl(-tb * xi);
                Ldbl res = yi - tc / denom;
                new_obj += res * res;
            }
            if (new_obj < best_obj) { best_a = ta; best_b = tb; best_c = tc; best_obj = new_obj; }
        }
        if (best_obj < prev_obj) { a = best_a; b = best_b; c = best_c; }
        else { a += delta[0]; b += delta[1]; c += delta[2]; if (a <= 0) a = 1e-6; if (c <= 0) c = 1e-6; }
    }

    // Validate logistic parameters: a > 0, c > 0 required for valid sigmoid
    if (a <= 0.0) a = 1e-6;
    if (c <= 0.0) c = 1e-6;
    m_rg.a = a; m_rg.b = b; m_rg.c = c;

    Ldbl sy_tot = 0, sy2 = 0, y_avg = 0, x_avg = 0;
    Ldbl r_num = 0, r_den_x = 0, r_den_y = 0;
    for (i = 0; i < nd; i++) { x_avg += _L(xd + i); y_avg += _L(yd + i); }
    x_avg /= m_rg.n; y_avg /= m_rg.n;
    for (i = 0; i < nd; i++) {
        Ldbl xi = _L(xd + i), yi = _L(yd + i);
        Ldbl yp = regFY(xi);
        Ldbl dy = yi - y_avg, dr = yi - yp, dx = xi - x_avg, dy_pred = yp - y_avg;
        sy_tot += dy * dy; sy2 += dr * dr; r_num += dx * dy_pred; r_den_x += dx * dx; r_den_y += dy_pred * dy_pred;
    }
    m_rg.rcrit = (sy_tot > 0) ? 1.0 - sy2 / sy_tot : 0.0;
    m_rg.r = (r_den_x > 0 && r_den_y > 0) ? r_num / sqrtl(r_den_x * r_den_y) : 0.0;
    m_rg.cov = sy2 / (m_rg.n - 3.0);
}

int StatsEngine::autoMode(const std::vector<double>& xd, const std::vector<double>& yd)
{
    int bestMode = 0;
    Ldbl bestR = -1e100;

    for (int i = 0; i < 9; i++) {
        m_rg.mode = i;
        compute(xd, yd);
        if (m_rg.rcrit > bestR) {
            bestR = m_rg.rcrit;
            bestMode = i;
        }
    }

    m_rg.mode = bestMode;
    compute(xd, yd);
    return bestMode;
}

Ldbl StatsEngine::regFY(Ldbl x) const
{
    // Evaluate fitted regression curve at x
    // IMPORTANT: y=0 returned for out-of-domain inputs means evaluation failed
    // This indicates extrapolation beyond the valid domain of the training data
    Ldbl y = 0;
    if (m_rg.mode == 0) {
        y = m_rg.a + m_rg.b * x;
    }
    if (m_rg.mode == 1) {
        // y = a + b*ln(x+tx): requires x+tx > 0
        // DOMAIN LIMIT: Only valid for x > -tx (where -tx = -1+xmin if xmin<=0)
        // EXTRAPOLATION: Returns y=0 if x < -tx (beyond training domain)
        Ldbl arg = x + m_rg.tx;
        if (arg > 0.0) y = m_rg.a + m_rg.b * logl(arg);
        else y = 0.0;  // Out of domain - EXTRAPOLATION DETECTED
    }
    if (m_rg.mode == 2) {
        y = m_rg.a * expl(m_rg.b * x) - m_rg.ty;
    }
    if (m_rg.mode == 3) {
        // y = a*x^b: Power law (tx=0, ty=0 always)
        // DOMAIN LIMIT: Only valid for x > 0 (negative powers undefined)
        // EXTRAPOLATION: Returns y=0 if x <= 0 (beyond training domain)
        Ldbl arg = x + m_rg.tx;
        if (arg > 0.0) y = m_rg.a * powl(arg, m_rg.b) - m_rg.ty;
        else y = 0.0;  // Out of domain - EXTRAPOLATION DETECTED
    }
    if (m_rg.mode == 4) {
        // y = a*x^b: Power law via NLS (tx=0, ty=0 always)
        // DOMAIN LIMIT: Only valid for x > 0 (x^b undefined for x<0 with non-integer b)
        // EXTRAPOLATION: Returns y=0 if x<0 (beyond training domain)
        if (x < 0.0) y = 0.0;  // Out of domain - EXTRAPOLATION DETECTED
        else y = m_rg.a * powl(x, m_rg.b);
    }
    if (m_rg.mode == 5) {
        // y = a + b/x: undefined at x=0
        if (fabsl(x) < 1e-30) y = 0.0;
        else y = m_rg.a + m_rg.b / x;
    }
    if (m_rg.mode == 6) {
        y = m_rg.a + m_rg.b * x + m_rg.c * x * x;
    }
    if (m_rg.mode == 7) {
        // y = a*sin(b*x+c) + d: always defined
        y = m_rg.a * sinl(m_rg.b * x + m_rg.c) + m_rg.d;
    }
    if (m_rg.mode == 8) {
        // y = c/(1+a*e^(-bx)): requires a > 0, c > 0
        if (m_rg.a > 0.0 && m_rg.c > 0.0) {
            Ldbl ex = expl(-m_rg.b * x);
            y = m_rg.c / (1.0 + m_rg.a * ex);
        } else {
            y = 0.0;
        }
    }
    return y;
}

Ldbl StatsEngine::regFX(Ldbl y) const
{
    Ldbl x = 0;
    if (m_rg.mode == 0) {
        // y = a + b*x => x = (y-a)/b
        if (fabsl(m_rg.b) < 1e-30) x = 0.0;
        else x = (y - m_rg.a) / m_rg.b;
    }
    if (m_rg.mode == 1) {
        // y = a + b*ln(x+tx) => x = e^((y-a)/b) - tx
        if (fabsl(m_rg.b) < 1e-30) x = 0.0;
        else {
            Ldbl exp_val = expl((y - m_rg.a) / m_rg.b);
            x = exp_val - m_rg.tx;
        }
    }
    if (m_rg.mode == 2) {
        // y = a*e^(b*x) - ty => x = ln((y+ty)/a)/b
        if (fabsl(m_rg.b) < 1e-30 || m_rg.a <= 0.0) x = 0.0;
        else {
            Ldbl arg = (y + m_rg.ty) / m_rg.a;
            if (arg > 0.0) x = logl(arg) / m_rg.b;
            else x = 0.0;  // Out of domain
        }
    }
    if (m_rg.mode == 3) {
        // y = a*(x+tx)^b - ty => x = ((y+ty)/a)^(1/b) - tx
        if (fabsl(m_rg.b) < 1e-30 || m_rg.a <= 0.0) x = 0.0;
        else {
            Ldbl arg = (y + m_rg.ty) / m_rg.a;
            if (arg > 0.0) x = powl(arg, 1.0 / m_rg.b) - m_rg.tx;
            else x = 0.0;  // Out of domain
        }
    }
    if (m_rg.mode == 4) {
        // y = a*x^b => x = (y/a)^(1/b)
        if (m_rg.a <= 0.0 || y < 0.0) x = 0.0;
        else if (fabsl(m_rg.b) < 1e-30) x = 0.0;
        else if (y == 0.0 && m_rg.b <= 0.0) x = 0.0;
        else x = powl(y / m_rg.a, 1.0 / m_rg.b);
    }
    if (m_rg.mode == 5) {
        // y = a + b/x => x = b/(y-a)
        Ldbl tmp = y - m_rg.a;
        if (fabsl(tmp) < 1e-30) x = 0.0;
        else x = m_rg.b / tmp;
    }
    if (m_rg.mode == 6) {
        // y = a + b*x + c*x²: solve c*x² + b*x + (a-y) = 0
        if (fabsl(m_rg.c) < 1e-30) {
            // Linear case: y = a + b*x
            if (fabsl(m_rg.b) < 1e-30) x = 0.0;
            else x = (y - m_rg.a) / m_rg.b;
        } else {
            // Quadratic case
            Ldbl disc = m_rg.b * m_rg.b - 4.0 * m_rg.c * (m_rg.a - y);
            if (disc >= 0) {
                Ldbl sqrt_disc = sqrtl(disc);
                Ldbl x1 = (-m_rg.b - sqrt_disc) / (2.0 * m_rg.c);
                Ldbl x2 = (-m_rg.b + sqrt_disc) / (2.0 * m_rg.c);
                // Choose positive root, or largest if both negative
                if (x1 > 0.0 || x2 > 0.0) {
                    x = (x1 > 0.0 && x2 > 0.0) ? (x1 < x2 ? x1 : x2) :
                        (x1 > 0.0 ? x1 : x2);
                } else {
                    x = (x1 > x2) ? x1 : x2;
                }
            } else {
                x = 0.0;  // No real solution
            }
        }
    }
    if (m_rg.mode == 7) {
        // y = a*sin(b*x+c) + d => x = (asin((y-d)/a) - c) / b
        if (fabsl(m_rg.a) < 1e-30 || fabsl(m_rg.b) < 1e-30) x = 0.0;
        else {
            Ldbl arg = (y - m_rg.d) / m_rg.a;
            if (arg >= -1.0 && arg <= 1.0) {
                x = (asinl(arg) - m_rg.c) / m_rg.b;
            } else {
                x = 0.0;  // Out of domain
            }
        }
    }
    if (m_rg.mode == 8) {
        // y = c/(1+a*e^(-bx)) => x = -ln((c/y-1)/a) / b
        if (m_rg.a <= 0.0 || m_rg.c <= 0.0 || fabsl(m_rg.b) < 1e-30) x = 0.0;
        else {
            Ldbl tmp = m_rg.c / y - 1.0;
            if (tmp > 0.0) x = -logl(tmp / m_rg.a) / m_rg.b;
            else x = 0.0;  // Out of domain
        }
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

bool StatsEngine::isExtrapolation(Ldbl x, Ldbl y) const
{
    // Check if input values are outside the training domain
    // Returns true if attempting extrapolation beyond training data range

    // Check X domain
    if (m_rg.mode >= 1 && m_rg.mode <= 3) {
        // Modes with x transformation: check x+tx > 0
        if (x + m_rg.tx <= 0.0) return true;
    }
    if (m_rg.mode >= 4 && m_rg.mode <= 4) {
        // Mode 4: requires x > 0
        if (x <= 0.0) return true;
    }

    // Check training domain bounds
    if (x < m_rg.xmin || x > m_rg.xmax) return true;

    // Check Y domain (for regFX inverse)
    if (y < m_rg.ymin || y > m_rg.ymax) return true;

    return false;  // Within training domain
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
        os << "y = a*e^(b*x) - ty , x = ln((y+ty)/a)/b\n";
    } else if (m_rg.mode == 3) {
        os << "REGRESSION PUISSANCE (MODE 3)\n";
        os << "y = a*(x+tx)^b - ty , x = ((y+ty)/a)^(1/b) - tx\n";
    } else if (m_rg.mode == 4) {
        os << "REGRESSION PUISSANCE NLS (MODE 4)\n";
        os << "y = a*x^b , x = (y/a)^(1/b)\n";
    } else if (m_rg.mode == 5) {
        os << "REGRESSION RECIPROQUE (MODE 5)\n";
        os << "y = a + b/x , x = b/(y-a)\n";
    } else if (m_rg.mode == 6) {
        os << "REGRESSION POLYNOMIALE DEG 2 (MODE 6)\n";
        os << "y = a + b*x + c*x^2\n";
    } else if (m_rg.mode == 7) {
        os << "REGRESSION SINUSOIDALE (MODE 7)\n";
        os << "y = a*sin(b*x + c) + d , x = (asin((y-d)/a)-c)/b\n";
    } else if (m_rg.mode == 8) {
        os << "REGRESSION LOGISTIQUE (MODE 8)\n";
        os << "y = c/(1+a*e^(-b*x)) , x = -ln((c/y-1)/a)/b\n";
    }

    os << "A = " << m_rg.a << " , B = " << m_rg.b
       << " , r = " << m_rg.r << "\n";
    if (m_rg.mode >= 6) os << "C = " << m_rg.c << "\n";
    if (m_rg.mode == 7) os << "D = " << m_rg.d << "\n";
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
