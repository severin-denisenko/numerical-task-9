//
// Created by Severin on 18.11.2022.
//

#include "NewtonSolver.h"

#include <cmath>
#include <utility>

NewtonSolver::NewtonSolver(const Context& context)
{
    m_context = context;
}

std::vector<double> NewtonSolver::solve_newton(std::function<std::vector<double>(std::vector<double>)> &f,
                                               const std::vector<double> &initial_guess, uint32_t max_iterations)
{
    size_type n = initial_guess.size();

    std::vector<double> previous = initial_guess;
    std::vector<double> next(n);

    for (uint32_t i = 0; i < max_iterations; ++i)
    {
        // n by n + 1 matrix: (derivatives | derivatives * previous - f(previous))
        std::vector<std::vector<double>> m;
        create(m, n, n + 1);

        // Find all derivatives (last column stay intact)
        derive(m, f, previous);

        // Find right part and put into m (compute last column)
        plug_vector(m, subtract(multiply(m, previous), f(previous)));

        solve(m, next);

        if (residual(previous, next) < m_context.newton_precision)
        {
            break;
        }

        std::swap(next, previous);
    }

    std::vector<double> output = next;

    return output;
}


double NewtonSolver::residual(const std::vector<double> &a, const std::vector<double> &b)
{
    double res = 0;
    size_type n = a.size();

    for (size_type i = 0; i < n; ++i)
    {
        res += (a[i] - b[i]) * (a[i] - b[i]);
    }

    return res;
}

void NewtonSolver::derive(std::vector<std::vector<double>> &m, std::function<std::vector<double>(std::vector<double>)> &f, const std::vector<double> &x) const
{
    size_type n = m.size();

    for (size_type column = 0; column < n; ++column)
    {
        std::vector<double> l = x;
        std::vector<double> r = x;
        l[column] += m_context.newton_derive_step;
        r[column] -= m_context.newton_derive_step;

        std::vector<double> d = divide(subtract(f(l), f(r)), 2 * m_context.newton_derive_step);

        for (size_type row = 0; row < n; ++row)
        {
            m[row][column] = d[row];
        }
    }
}

void NewtonSolver::solve(std::vector<std::vector<double>> &mx, std::vector<double> &res)
{
    size_type n = mx.size();
    std::vector<size_type> x_order(n);

    for (size_type i = 0; i < n; i++)
    {
        x_order[i] = i;
    }

    for (size_type j = 0; j < n; j++)
    {
        // Serch for biggest element and change order //

        size_type max_i = 0;
        size_type max_j = 0;
        double max = 0;

        for (size_type z = j; z < n; z++)
        {
            for (size_type t = j; t < n; t++)
            {
                if (fabs(mx[z][t]) > max)
                {
                    max = fabs(mx[z][t]);
                    max_i = z;
                    max_j = t;
                }
            }
        }

        for (size_type i = 0; i < n; i++)
        {
            std::swap(mx[i][max_j], mx[i][j]);
        }
        std::swap(mx[max_i], mx[j]);
        std::swap(x_order[j], x_order[max_j]);

        ////////////////////////////////////////////////

        for (size_type i = j + 1; i < n; i++)
        {
            double div = mx[i][j] / mx[j][j];

            for (size_type k = 0; k < n + 1; k++)
            {
                mx[i][k] = mx[i][k] - div * mx[j][k];
            }
        }
    }

    res[n - 1] = mx[n - 1][n] / mx[n - 1][n - 1];

    for (int i = (int)n - 2; i >= 0; i--)
    {
        double sum = 0;
        for (int j = i + 1; j < (int)n; j++)
        {
            sum = sum + mx[i][j] * res[j];
        }
        res[i] = (mx[i][n] - sum) / mx[i][i];
    }

    // Restore the order of res //

    for (size_type i = 0; i < n; i++) {
        size_type next = i;

        while (x_order[next] != n) {

            std::swap(res[i], res[x_order[next]]);

            size_type temp = x_order[next];

            x_order[next] = n;
            next = temp;
        }
    }
}

NewtonSolver::NewtonSolver()
{
    m_context = Context();
}
