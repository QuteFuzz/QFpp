#include <circuit.h>
#include <rule.h>

/// Box-Muller normal sample
static float random_normal() {
    float u1 = std::max(random_float(1.0f, 0.0f), 1e-7f);
    float u2 = random_float(1.0f, 0.0f);
    return std::sqrt(-2.0f * std::log(u1)) * std::cos(2.0f * std::numbers::pi * u2);
}

/// Gram-Schmidt column orthogonalisation to make unitary matrix
/// Sufficient for fuzzing; for strict Haar measure apply QR phase correction too.
static CxMat haar_unitary(unsigned int n) {
    CxMat src(n, std::vector<Cx>(n));
    for (auto& row : src)
        for (auto& c : row)
            c = Cx(random_normal(), random_normal());

    CxMat q(n, std::vector<Cx>(n));

    for (unsigned int j = 0; j < n; j++) {
        std::vector<Cx> col(n);
        for (unsigned int i = 0; i < n; i++) col[i] = src[i][j];

        // subtract projections onto previous orthonormal columns
        for (unsigned int k = 0; k < j; k++) {
            Cx proj = 0;
            for (unsigned int i = 0; i < n; i++)
                proj += std::conj(q[i][k]) * col[i];
            for (unsigned int i = 0; i < n; i++)
                col[i] -= proj * q[i][k];
        }

        // normalise
        float norm = 0;
        for (unsigned int i = 0; i < n; i++)
            norm += std::norm(col[i]);     // std::norm = |c|^2
        norm = std::sqrt(norm);

        for (unsigned int i = 0; i < n; i++)
            q[i][j] = col[i] / norm;
    }

    return q;
}

Circuit::Circuit() :
    Cloneable<Circuit>("circuit", CIRCUIT),
    name("dummy_circuit")
{}

/// @brief Generating a random circuit from scratch
Circuit::Circuit(std::string _name, Token_kind kind) :
    Cloneable<Circuit>("circuit", kind),
    name(_name)
{}

/// @brief Generating a random unitary from scratch
Circuit::Circuit(std::string _name, unsigned int _n_matrix_qubits) :
    Cloneable<Circuit>("circuit", (_n_matrix_qubits == 2 ? UNITARY_2Q_DEF : UNITARY_1Q_DEF)),
    name(_name),
    n_matrix_qubits(_n_matrix_qubits),
    dim(1u << _n_matrix_qubits),
    matrix(std::move(haar_unitary(dim)))
{}

std::string Circuit::get_val_at(int row, int col) const {
    std::string out = "";

    auto u_row = (unsigned int)std::max(row, 0);
    auto u_col = (unsigned int)std::max(col, 0);

    if ((u_row < dim) && (u_col < dim)){
        float re = matrix[u_row][u_col].real();
        float im = matrix[u_row][u_col].imag();
        
        out = std::to_string(re);

        if (im >= 0.0f){
            out += "+";
        }

        out += std::to_string(im);
    }

    return out;
}

void Circuit::print_info() const {

    std::cout << "=======================================" << std::endl;
    std::cout << "              CIRCUIT INFO               " << std::endl;
    std::cout << "=======================================" << std::endl;
    std::cout << "Name: " << name << std::endl;
    std::cout << "---> " << this << std::endl;

    if (kind == UNITARY_1Q_DEF){
        std::cout << "Custom 1q unitary" << std::endl;
    } else if (kind == UNITARY_2Q_DEF){
        std::cout << "Custom 2q unitary" << std::endl;
    } else {
        std::cout << "Resource defs" << std::endl;

        for(const auto& rd : resource_defs){
            rd->print_info();
        }

        std::cout << "Resources " << std::endl;

        for(const auto& r : resources){
            r->print_info();
        }
    }

    std::cout << "=======================================" << std::endl;
}
