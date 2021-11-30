/*
 * Author Telnov Victor, v-telnov@yandex.ru
 * This code under licence GPL3
 */

#include <cmath>
#include <cassert>
#include <utility>

using std::fabs;

using uint = unsigned int;
using Extended = long double;
#define EE(X) static_cast<Extended>(X)

double bernoulli(uint k, uint n, double probability) {

    assert( n > 0 && k <= n );
    assert( probability >= 0.0 && probability <= 1.0 );

    if (probability == 0.0) {
        if (k == 0) return 1.0;
        return 0.0;

    } else if (probability == 1.0) {
        if (k == n) return 1.0;
        return 0.0;

    } else if (k == 0)
        return pow(1.0 - probability, n);

    else if (k == n)
        return pow(probability, k);

    Extended need_p = probability;
    Extended back_p = 1.0 - need_p;

    uint c_need_p = k;
    uint c_back_p = n - k;

    if ( n > 2 && k > n/2 )
        k = n - k;

    uint divider = k;
    uint multipler = n - k;

    Extended r = 1.0;

    for (;;) {

        while ( r <= 1.0 && multipler < n )
            r = r * EE(++multipler);

        if ( divider > 0 )
            r = r / EE(divider--);

        else if ( c_need_p > 0 ) {
            r = r * need_p;
            --c_need_p;

        } else if ( c_back_p > 0 ) {
            r = r * back_p;
            --c_back_p;

        } else {
            assert( multipler == n );
            break;
        }
    }

    return r;
}

std::pair<bool,double> recu__bernoulli_integral_inverse(
        double er,
        uint k, uint n,
        double x1, double y1, double x2, double y2,
        double& s_resi) {

    double dx = fabs(x2 - x1);
    if (dx == 0.0) return {false,0};

    double dy = fabs(y2 - y1);
    double square_er = dx * dy;

    if (square_er > er / dx) {

        double mx = (x1 + x2) / 2.0;
        double my = bernoulli(k, n, mx);

        auto x_res = recu__bernoulli_integral_inverse(er, k, n, x1, y1, mx, my, s_resi);

        if (!x_res.first)
            x_res = recu__bernoulli_integral_inverse(er, k, n, mx, my, x2, y2, s_resi);

        return x_res;
    }

    double s = (y1 + y2) / 2.0 * dx;

    if (s < s_resi) {
        s_resi -= s;
        return {false,0};
    }

    double a = (y2 - y1) / (x2 - x1);
    double b = y1 - a * x1;

    s = (b + y1) / 2.0 * x1;

    if (x1 < x2)
        s = s + s_resi;
    else
        s = s - s_resi;

    return {true, ( -b + sqrt(b * b + 2.0 * a * s) ) / a};
}

double bernoulli_integral_inverse(uint k, uint n, double p_quantile, bool dir_right_to_left, double er) {

    double inner_er = er / (double)(n+1);

    double mx = (double)k / (double)n;
    double my = bernoulli(k, n, mx);

    double st_x = 0;
    double en_x = 1;

    if (dir_right_to_left) {
        st_x = 1;
        en_x = 0;
    }

    p_quantile /= (double)(n+1);

    auto x_res = recu__bernoulli_integral_inverse(inner_er, k, n, st_x, 0, mx, my, p_quantile);

    if (!x_res.first)
        x_res = recu__bernoulli_integral_inverse(inner_er, k, n, mx, my, en_x, 0, p_quantile);

    assert( x_res.first );

    return x_res.second;
}
