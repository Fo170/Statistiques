#ifndef STATSENGINE_H
#define STATSENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstdio>

typedef long double Ldbl;
typedef unsigned int uint;
typedef unsigned long ulong;

#define pw2(x) ((x)*(x))
#define _L(x)  ((Ldbl)*(x))
#define _(x)   (*(x))

enum RegMode {
    REG_LINEAIRE       = 0,
    REG_LOGARITHMIQUE  = 1,
    REG_EXPONENTIELLE  = 2,
    REG_PUISSANCE      = 3,
    REG_PUISSANCE_NLS  = 4
};

struct RG_t {
    int mode;
    Ldbl xmin, xmax, ymin, ymax;
    Ldbl tx, ty, n, a, b, r, rcrit, cov;
    Ldbl sx, sy, sxy, sx2, sy2;
};

class StatsEngine {
public:
    StatsEngine();

    void setMode(int mode);
    int mode() const;

    void compute(const std::vector<double>& xd, const std::vector<double>& yd);
    int autoMode(const std::vector<double>& xd, const std::vector<double>& yd);

    Ldbl regFY(Ldbl x) const;
    Ldbl regFX(Ldbl y) const;

    void printResults() const;
    std::string resultsText() const;

    const RG_t& rg() const { return m_rg; }

    void sortData(std::vector<double>& xd, std::vector<double>& yd);

private:
    RG_t m_rg;
    void regTpl(const double* xd, const double* yd, ulong nd);
    void regNls(const double* xd, const double* yd, ulong nd);
};

#endif
