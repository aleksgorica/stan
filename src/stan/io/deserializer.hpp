#ifndef STAN_IO_DESERIALIZER_HPP
#define STAN_IO_DESERIALIZER_HPP

#include <stan/math/rev.hpp>

namespace stan {

namespace io {

/**
 * A stream-based reader for integer, scalar, vector, matrix
 * and array data types, with Jacobian calculations.
 *
 * The template parameter <code>T</code> represents the type of
 * scalars and the values in vectors and matrices.  The only
 * requirement on the template type <code>T</code> is that a
 * double can be copied into it, as in
 *
 * <code>T t = 0.0;</code>
 *
 * This includes <code>double</code> itself and the reverse-mode
 * algorithmic differentiation class <code>stan::math::var</code>.
 *
 * <p>For transformed values, the scalar type parameter <code>T</code>
 * must support the transforming operations, such as <code>exp(x)</code>
 * for positive-bounded variables.  It must also support equality and
 * inequality tests with <code>double</code> values.
 *
 * @tparam T Basic scalar type.
 */
template <typename T>
class deserializer {
 private:
  Eigen::Map<const Eigen::Matrix<T, -1, 1>> data_r_;
  Eigen::Map<const Eigen::Matrix<int, -1, 1>> data_i_;
  size_t r_size_{0};
  size_t i_size_{0};
  size_t pos_r_{0};
  size_t pos_i_{0};

  inline const T& scalar_ptr() { return data_r_.coeffRef(pos_r_); }

  inline const T& scalar_ptr_increment(size_t m) {
    pos_r_ += m;
    return data_r_.coeffRef(pos_r_ - m);
  }

  inline const int& int_ptr() { return data_i_.coeffRef(pos_i_); }

  inline const int& int_ptr_increment(size_t m) {
    pos_i_ += m;
    return data_i_.coeffRef(pos_i_ - m);
  }
  void check_r_capacity() const {
    if (pos_r_ >= r_size_) {
      []() STAN_COLD_PATH {
        throw std::runtime_error("no more scalars to read");
      }();
    }
  }

  void check_i_capacity() const {
    if (pos_i_ >= i_size_) {
      []() STAN_COLD_PATH {
        throw std::runtime_error("no more integers to read");
      }();
    }
  }

 public:
  using matrix_t = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;
  using vector_t = Eigen::Matrix<T, Eigen::Dynamic, 1>;
  using row_vector_t = Eigen::Matrix<T, 1, Eigen::Dynamic>;

  using map_matrix_t = Eigen::Map<const matrix_t>;
  using map_vector_t = Eigen::Map<const vector_t>;
  using map_row_vector_t = Eigen::Map<const row_vector_t>;

  using var_matrix_t = stan::math::var_value<
      Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>>;
  using var_vector_t
      = stan::math::var_value<Eigen::Matrix<double, Eigen::Dynamic, 1>>;
  using var_row_vector_t
      = stan::math::var_value<Eigen::Matrix<double, 1, Eigen::Dynamic>>;

  /**
   * Construct a variable reader using the specified vectors
   * as the source of scalar and integer values for data.  This
   * class holds a reference to the specified data vectors.
   *
   * Attempting to read beyond the end of the data or integer
   * value sequences raises a runtime exception.
   *
   * @param data_r Sequence of scalar values.
   * @param data_i Sequence of integer values.
   */
  template <typename RVec, typename IntVec,
            require_all_vector_like_t<RVec, IntVec>* = nullptr>
  deserializer(RVec& data_r, IntVec& data_i)
      : data_r_(data_r.data(), data_r.size()),
        data_i_(data_i.data(), data_i.size()),
        r_size_(data_r.size()),
        i_size_(data_i.size()) {}

  /**
   * Return the number of scalars remaining to be read.
   *
   * @return Number of scalars left to read.
   */
  inline size_t available() const noexcept { return r_size_ - pos_r_; }

  /**
   * Return the number of integers remaining to be read.
   *
   * @return Number of integers left to read.
   */
  inline size_t available_i() const noexcept { return i_size_ - pos_i_; }

  template <typename Ret, require_any_t<std::is_floating_point<Ret>,
                                        is_autodiff<Ret>>* = nullptr>
  auto read() {
    check_r_capacity();
    return data_r_.coeffRef(pos_r_++);
  }

  template <typename Ret, require_complex_t<Ret>* = nullptr>
  auto read() {
    check_r_capacity();
    return std::complex<T>(data_r_.coeffRef(pos_r_++),
                           data_r_.coeffRef(pos_r_++));
  }

  template <typename Ret, require_integral_t<Ret>* = nullptr>
  auto read() {
    check_i_capacity();
    return data_i_.coeffRef(pos_i_++);
  }

  template <typename Ret, typename Size,
            require_eigen_col_vector_t<Ret>* = nullptr>
  auto read(Size m) {
    if (unlikely(m == 0)) {
      return map_vector_t(nullptr, m);
    } else {
      return map_vector_t(&scalar_ptr_increment(m), m);
    }
  }

  template <typename Ret, typename Size,
            require_eigen_row_vector_t<Ret>* = nullptr>
  auto read(Size m) {
    if (unlikely(m == 0)) {
      return map_row_vector_t(nullptr, m);
    } else {
      return map_row_vector_t(&scalar_ptr_increment(m), m);
    }
  }

  template <typename Ret, typename Rows, typename Cols,
            require_eigen_matrix_dynamic_t<Ret>* = nullptr>
  auto read(Rows rows, Cols cols) {
    if (rows == 0 || cols == 0) {
      return map_matrix_t(nullptr, rows, cols);
    } else {
      return map_matrix_t(&scalar_ptr_increment(rows * cols), rows, cols);
    }
  }

  template <typename Ret, typename T_ = T, typename... Sizes,
            require_var_t<T_>* = nullptr, require_var_matrix_t<Ret>* = nullptr>
  auto read(Sizes... sizes) {
    using value_t = typename Ret::value_type;
    return stan::math::to_var_value(this->read<value_t>(sizes...));
  }

  template <typename Ret, typename T_ = T, typename... Sizes,
            require_not_var_t<T_>* = nullptr,
            require_var_matrix_t<Ret>* = nullptr>
  auto read(Sizes... sizes) {
    using value_t = typename Ret::value_type;
    return this->read<value_t>(sizes...);
  }

  template <typename Ret, typename Size, typename... Sizes,
            require_std_vector_t<Ret>* = nullptr>
  inline auto read(Size m, Sizes... dims) {
    if (unlikely(m == 0)) {
      return Ret();
    } else {
      using ret_value_type = value_type_t<Ret>;
      Ret ret_vec;
      ret_vec.reserve(m);
      for (Size i = 0; i < m; ++i) {
        ret_vec.emplace_back(this->read<ret_value_type>(dims...));
      }
      return ret_vec;
    }
  }

  template <typename Ret, typename LB, typename... Sizes>
  auto read_lb(const LB& lb, Sizes... sizes) {
    using stan::math::check_greater_or_equal;
    auto ret = this->read<Ret>(sizes...);
    check_greater_or_equal("io deserializer", "Lower Bound", ret, lb);
    return ret;
  }

  template <typename Ret, bool Jacobian, typename LB, typename LP,
            typename... Sizes>
  auto read_lb_constrain(const LB& lb, LP& lp, Sizes... sizes) {
    if (Jacobian) {
      return stan::math::lb_constrain(this->read<Ret>(sizes...), lb, lp);
    } else {
      return stan::math::lb_constrain(this->read<Ret>(sizes...), lb);
    }
  }

  template <typename Ret, typename UB, typename... Sizes>
  auto read_ub(const UB& ub, Sizes... sizes) {
    using stan::math::check_less_or_equal;
    auto ret = this->read<Ret>(sizes...);
    check_less_or_equal("io deserializer", "Upper Bound", ret, ub);
    return ret;
  }

  template <typename Ret, bool Jacobian, typename UB, typename LP,
            typename... Sizes>
  auto read_ub_constrain(const UB& ub, LP& lp, Sizes... sizes) {
    if (Jacobian) {
      return stan::math::ub_constrain(this->read<Ret>(sizes...), ub, lp);
    } else {
      return stan::math::ub_constrain(this->read<Ret>(sizes...), ub);
    }
  }

  template <typename Ret, typename LB, typename UB, typename... Sizes>
  auto read_lub(const LB& lb, const UB& ub, Sizes... sizes) {
    using stan::math::check_bounded;
    auto ret = this->read<Ret>(sizes...);
    check_bounded<Ret, LB, UB>("io deserializer", "Upper and Lower Bound", ret,
                               lb, ub);
    return ret;
  }

  template <typename Ret, bool Jacobian, typename LB, typename UB, typename LP,
            typename... Sizes>
  auto read_lub_constrain(const LB& lb, const UB& ub, LP& lp, Sizes... sizes) {
    if (Jacobian) {
      return stan::math::lub_constrain(this->read<Ret>(sizes...), lb, ub, lp);
    } else {
      return stan::math::lub_constrain(this->read<Ret>(sizes...), lb, ub);
    }
  }

  template <typename Ret, typename... Sizes>
  auto read_pos(const Sizes&... sizes) {
    auto ret = read<Ret>(sizes...);
    stan::math::check_positive("stan::io::deserializer", "Positive Constrained",
                               ret);
    return ret;
  }
  template <typename Ret, bool Jacobian, typename LP, typename... Sizes,
            require_not_std_vector_t<Ret>* = nullptr>
  auto read_pos(LP& lp, const Sizes&... sizes) {
    if (Jacobian) {
      return stan::math::positive_constrain(this->read<Ret>(sizes...), lp);
    } else {
      return stan::math::positive_constrain(this->read<Ret>(sizes...));
    }
  }

  template <typename Ret, bool Jacobian, typename LP, typename... Sizes,
            require_std_vector_t<Ret>* = nullptr>
  auto read_pos(LP& lp, const size_t vecsize, const Sizes&... sizes) {
    using stan::math::positive_constrain;
    using ret_value_t = value_type_t<Ret>;
    std::decay_t<Ret> ret;
    for (size_t i = 0; i < vecsize; ++i) {
      ret.emplace_back(this->read_pos<ret_value_t, Jacobian>(lp, sizes...));
    }
    return ret;
  }

  template <typename Ret, typename Offset, typename Mult, typename... Sizes>
  auto read_offset_multiplier(const Offset& offset, const Mult& multiplier,
                              const Sizes&... sizes) {
    return read<Ret>(sizes...);
  }

  template <typename Ret, bool Jacobian, typename Offset, typename Mult,
            typename LP, typename... Sizes,
            require_not_std_vector_t<Ret>* = nullptr>
  auto read_offset_multiplier(const Offset& offset, const Mult& multiplier, LP& lp,
                const Sizes&... sizes) {
    using stan::math::offset_multiplier_constrain;
    if (Jacobian) {
      return offset_multiplier_constrain(this->read<Ret>(sizes...), offset,
                                         multiplier, lp);
    } else {
      return offset_multiplier_constrain(this->read<Ret>(sizes...), offset,
                                         multiplier);
    }
  }

  template <typename Ret, bool Jacobian, typename Offset, typename Mult,
            typename LP, typename... Sizes,
            require_std_vector_t<Ret>* = nullptr>
  auto read_offset_multiplier(const Offset& offset, const Mult& multiplier,
                              LP& lp, const size_t vecsize,
                              const Sizes&... sizes) {
    using stan::math::offset_multiplier_constrain;
    using ret_value_t = value_type_t<Ret>;
    std::decay_t<Ret> ret;
    for (size_t i = 0; i < vecsize; ++i) {
      ret.emplace_back(this->read_offset_multiplier<ret_value_t, Jacobian>(
          offset, multiplier, lp, sizes...));
    }
    return ret;
  }

  template <typename Ret, typename... Sizes>
  auto read_prob(const Sizes&... sizes) {
    auto ret = read<Ret>(sizes...);
    stan::math::check_bounded<Ret, double, double>(
        "stan::io::deserializer", "Constrained probability", ret, 0, 1);
    return ret;
  }
  template <typename Ret, bool Jacobian, typename LP, typename... Sizes,
            require_not_std_vector_t<Ret>* = nullptr>
  auto read_prob(LP& lp, const Sizes&... sizes) {
    if (Jacobian) {
      return stan::math::prob_constrain(this->read<Ret>(sizes...), lp);
    } else {
      return stan::math::prob_constrain(this->read<Ret>(sizes...));
    }
  }

  template <typename Ret, bool Jacobian, typename LP, typename... Sizes,
            require_std_vector_t<Ret>* = nullptr>
  auto read_prob(LP& lp, const size_t vecsize, const Sizes&... sizes) {
    using stan::math::prob_constrain;
    using ret_value_t = value_type_t<Ret>;
    std::decay_t<Ret> ret;
    for (size_t i = 0; i < vecsize; ++i) {
      ret.emplace_back(this->read_prob<ret_value_t, Jacobian>(lp, sizes...));
    }
    return ret;
  }

  template <typename Ret, typename... Sizes,
            require_not_matrix_t<Ret>* = nullptr>
  auto read_corr(const Sizes&... sizes) {
    auto ret = read<Ret>(sizes...);
    stan::math::check_bounded<T, double, double>(
        "stan::io::deserializer", "Correlation value", ret, -1, 1);
    return ret;
  }

  template <typename Ret, require_matrix_t<Ret>* = nullptr>
  auto read_corr(size_t k) {
    using stan::math::check_corr_matrix;
    auto ret = read<Ret>(k, k);
    check_corr_matrix("stan::math::corr_matrix", "Constrained matrix", ret);
    return ret;
  }

  template <typename Ret, bool Jacobian, typename LP, typename... Sizes,
            require_not_std_vector_t<Ret>* = nullptr,
            require_not_matrix_t<Ret>* = nullptr>
  auto read_corr(LP& lp, const Sizes&... sizes) {
    using stan::math::corr_constrain;
    if (Jacobian) {
      return corr_constrain(this->read<Ret>(sizes...), lp);
    } else {
      return corr_constrain(this->read<Ret>(sizes...));
    }
  }

  template <typename Ret, bool Jacobian, typename LP,
            require_not_std_vector_t<Ret>* = nullptr,
            require_matrix_t<Ret>* = nullptr>
  auto read_corr(LP& lp, size_t k) {
    using stan::math::corr_matrix_constrain;
    if (Jacobian) {
      return corr_matrix_constrain(this->read<vector_t>((k * (k - 1)) / 2), k,
                                   lp);
    } else {
      return corr_matrix_constrain(this->read<vector_t>((k * (k - 1)) / 2), k);
    }
  }

  template <typename Ret, bool Jacobian, typename LP, typename... Sizes,
            require_std_vector_t<Ret>* = nullptr>
  auto read_corr(LP& lp, const size_t vecsize, const Sizes&... sizes) {
    using ret_value_t = value_type_t<Ret>;
    std::decay_t<Ret> ret;
    for (size_t i = 0; i < vecsize; ++i) {
      ret.emplace_back(this->read_corr<ret_value_t, Jacobian>(lp, sizes...));
    }
    return ret;
  }

  template <typename Ret, require_vector_t<Ret>* = nullptr>
  auto read_unit_vector(size_t k) {
    if (unlikely(k == 0)) {
      std::string msg = "deserializer: unit vectors cannot be size 0.";
      throw std::invalid_argument(msg);
    }
    auto ret = read<Ret>(k);
    stan::math::check_unit_vector("stan::io::deserializer", "Unit Vector", ret);
    return ret;
  }

  template <typename Ret, typename... Sizes,
            require_std_vector_t<Ret>* = nullptr>
  auto read_unit_vector(size_t vecsize, Sizes... sizes) {
    using ret_value_t = value_type_t<Ret>;
    std::decay_t<Ret> ret;
    for (size_t i = 0; i < vecsize; ++i) {
      ret.emplace_back(this->read_unit_vector<ret_value_t>(sizes...));
    }
    return ret;
  }

  template <typename Ret, bool Jacobian, typename LP, typename... Sizes,
            require_not_std_vector_t<Ret>* = nullptr>
  auto read_unit_vector(LP& lp, const Sizes&... sizes) {
    using stan::math::unit_vector_constrain;
    if (Jacobian) {
      return unit_vector_constrain(this->read<Ret>(sizes...), lp);
    } else {
      return unit_vector_constrain(this->read<Ret>(sizes...));
    }
  }

  template <typename Ret, bool Jacobian, typename LP, typename... Sizes,
            require_std_vector_t<Ret>* = nullptr>
  auto read_unit_vector(LP& lp, const size_t vecsize, const Sizes&... sizes) {
    using stan::math::unit_vector_constrain;
    using ret_value_t = value_type_t<Ret>;
    std::decay_t<Ret> ret;
    for (size_t i = 0; i < vecsize; ++i) {
      ret.emplace_back(
          this->read_unit_vector<ret_value_t, Jacobian>(lp, sizes...));
    }
    return ret;
  }

  template <typename Ret, require_vector_t<Ret>* = nullptr>
  auto read_simplex(size_t k) {
    if (unlikely(k == 0)) {
      std::string msg = "deserializer: simplex vectors cannot be size 0.";
      throw std::invalid_argument(msg);
    }
    auto ret = read<Ret>(k);
    stan::math::check_simplex("stan::io::deserializer", "Simplex", ret);
    return ret;
  }

  template <typename Ret, typename... Sizes,
            require_std_vector_t<Ret>* = nullptr>
  auto read_simplex(size_t vecsize, Sizes... sizes) {
    using ret_value_t = value_type_t<Ret>;
    std::decay_t<Ret> ret;
    for (size_t i = 0; i < vecsize; ++i) {
      ret.emplace_back(this->read_simplex<ret_value_t>(sizes...));
    }
    return ret;
  }

  template <typename Ret, bool Jacobian, typename LP, typename... Sizes,
            require_not_std_vector_t<Ret>* = nullptr>
  auto read_simplex(LP& lp, const Sizes&... sizes) {
    using stan::math::simplex_constrain;
    if (Jacobian) {
      return simplex_constrain(this->read<Ret>(sizes...), lp);
    } else {
      return simplex_constrain(this->read<Ret>(sizes...));
    }
  }

  template <typename Ret, bool Jacobian, typename LP, typename... Sizes,
            require_std_vector_t<Ret>* = nullptr>
  auto read_simplex(LP& lp, const size_t vecsize, const Sizes&... sizes) {
    using stan::math::simplex_constrain;
    using ret_value_t = value_type_t<Ret>;
    std::decay_t<Ret> ret;
    for (size_t i = 0; i < vecsize; ++i) {
      ret.emplace_back(this->read_simplex<ret_value_t, Jacobian>(lp, sizes...));
    }
    return ret;
  }

  template <typename Ret, typename... Sizes>
  auto read_ordered(const Sizes&... sizes) {
    using stan::math::check_ordered;
    auto ret = read<Ret>(sizes...);
    check_ordered("stan::io::deserializer", "Ordered", ret);
    return ret;
  }

  template <typename Ret, bool Jacobian, typename LP, typename... Sizes,
            require_not_std_vector_t<Ret>* = nullptr>
  auto read_ordered(LP& lp, const Sizes&... sizes) {
    using stan::math::ordered_constrain;
    if (Jacobian) {
      return ordered_constrain(this->read<Ret>(sizes...), lp);
    } else {
      return ordered_constrain(this->read<Ret>(sizes...));
    }
  }

  template <typename Ret, bool Jacobian, typename LP, typename... Sizes,
            require_std_vector_t<Ret>* = nullptr>
  auto read_ordered(LP& lp, const size_t vecsize, const Sizes&... sizes) {
    using stan::math::ordered_constrain;
    using ret_value_t = value_type_t<Ret>;
    std::decay_t<Ret> ret;
    for (size_t i = 0; i < vecsize; ++i) {
      ret.emplace_back(this->read_ordered<ret_value_t, Jacobian>(lp, sizes...));
    }
    return ret;
  }

  template <typename Ret, typename... Sizes>
  auto read_positive_ordered(const Sizes&... sizes) {
    using stan::math::check_positive_ordered;
    auto ret = read<Ret>(sizes...);
    check_positive_ordered("stan::io::deserializer", "Positive Ordered", ret);
    return ret;
  }

  template <typename Ret, bool Jacobian, typename LP, typename... Sizes,
            require_not_std_vector_t<Ret>* = nullptr>
  auto read_positive_ordered(LP& lp, const Sizes&... sizes) {
    using stan::math::positive_ordered_constrain;
    if (Jacobian) {
      return positive_ordered_constrain(this->read<Ret>(sizes...), lp);
    } else {
      return positive_ordered_constrain(this->read<Ret>(sizes...));
    }
  }

  template <typename Ret, bool Jacobian, typename LP, typename... Sizes,
            require_std_vector_t<Ret>* = nullptr>
  auto read_positive_ordered(LP& lp, const size_t vecsize,
                             const Sizes&... sizes) {
    using stan::math::positive_ordered_constrain;
    using ret_value_t = value_type_t<Ret>;
    std::decay_t<Ret> ret;
    for (size_t i = 0; i < vecsize; ++i) {
      ret.emplace_back(
          this->read_positive_ordered<ret_value_t, Jacobian>(lp, sizes...));
    }
    return ret;
  }

  template <typename Ret, require_matrix_t<Ret>* = nullptr>
  auto read_cholesky_factor_cov(size_t M, size_t N) {
    using stan::math::check_cholesky_factor;
    auto ret = read<Ret>(M, N);
    check_cholesky_factor("stan::io::deserializer", "Cholesky Factor Cov", ret);
    return ret;
  }

  template <typename Ret, typename... Sizes,
            require_std_vector_t<Ret>* = nullptr>
  auto read_cholesky_factor_cov(size_t vecsize, Sizes... sizes) {
    using ret_value_t = value_type_t<Ret>;
    std::decay_t<Ret> ret;
    for (size_t i = 0; i < vecsize; ++i) {
      ret.emplace_back(this->read_cholesky_factor_cov<ret_value_t>(sizes...));
    }
    return ret;
  }

  template <typename Ret, bool Jacobian, typename LP,
            require_matrix_t<Ret>* = nullptr>
  auto read_cholesky_factor_cov(LP& lp, size_t M, size_t N) {
    if (Jacobian) {
      return stan::math::cholesky_factor_constrain(
          this->read<vector_t>((N * (N + 1)) / 2 + (M - N) * N), M, N, lp);
    } else {
      return stan::math::cholesky_factor_constrain(
          this->read<vector_t>((N * (N + 1)) / 2 + (M - N) * N), M, N);
    }
  }

  template <typename Ret, bool Jacobian, typename LP, typename... Sizes,
            require_std_vector_t<Ret>* = nullptr>
  auto read_cholesky_factor_cov(LP& lp, const size_t vecsize,
                                const Sizes&... sizes) {
    using ret_value_t = value_type_t<Ret>;
    std::decay_t<Ret> ret;
    for (size_t i = 0; i < vecsize; ++i) {
      ret.emplace_back(
          this->read_cholesky_factor_cov<ret_value_t, Jacobian>(lp, sizes...));
    }
    return ret;
  }

  // SDF

  template <typename Ret, require_matrix_t<Ret>* = nullptr>
  auto read_cholesky_factor_corr(size_t M, size_t N) {
    using stan::math::check_cholesky_factor;
    auto ret = read<Ret>(M, N);
    check_cholesky_factor_corr("stan::io::cholesky_factor_corr",
                               "Constrained matrix", ret);
    return ret;
  }

  template <typename Ret, typename... Sizes,
            require_std_vector_t<Ret>* = nullptr>
  auto read_cholesky_factor_corr(size_t vecsize, Sizes... sizes) {
    using ret_value_t = value_type_t<Ret>;
    std::decay_t<Ret> ret;
    for (size_t i = 0; i < vecsize; ++i) {
      ret.emplace_back(this->read_cholesky_factor_corr<ret_value_t>(sizes...));
    }
    return ret;
  }

  template <typename Ret, bool Jacobian, typename LP,
            require_matrix_t<Ret>* = nullptr>
  auto read_cholesky_factor_corr(LP& lp, size_t K) {
    using stan::math::cholesky_corr_constrain;
    if (Jacobian) {
      return cholesky_corr_constrain(this->read<vector_t>((K * (K - 1)) / 2), K,
                                     lp);
    } else {
      return cholesky_corr_constrain(this->read<vector_t>((K * (K - 1)) / 2),
                                     K);
    }
  }

  template <typename Ret, bool Jacobian, typename LP, typename... Sizes,
            require_std_vector_t<Ret>* = nullptr>
  auto read_cholesky_factor_corr(LP& lp, const size_t vecsize,
                                 const Sizes&... sizes) {
    using ret_value_t = value_type_t<Ret>;
    std::decay_t<Ret> ret;
    for (size_t i = 0; i < vecsize; ++i) {
      ret.emplace_back(
          this->read_cholesky_factor_corr<ret_value_t, Jacobian>(lp, sizes...));
    }
    return ret;
  }

  // cov matrix

  template <typename Ret, require_matrix_t<Ret>* = nullptr>
  auto read_cov_matrix(size_t k) {
    using stan::math::check_cov_matrix;
    auto ret = read<Ret>(k, k);
    check_cov_matrix("stan::io::cov_matrix", "Constrained matrix", ret);
    return ret;
  }

  template <typename Ret, typename... Sizes,
            require_std_vector_t<Ret>* = nullptr>
  auto read_cov_matrix(size_t vecsize, Sizes... sizes) {
    using ret_value_t = value_type_t<Ret>;
    std::decay_t<Ret> ret;
    for (size_t i = 0; i < vecsize; ++i) {
      ret.emplace_back(this->read_cholesky_factor_corr<ret_value_t>(sizes...));
    }
    return ret;
  }

  template <typename Ret, bool Jacobian, typename LP,
            require_matrix_t<Ret>* = nullptr>
  auto read_cov_matrix(LP& lp, size_t k) {
    using stan::math::cov_matrix_constrain;
    if (Jacobian) {
      return cov_matrix_constrain(this->read<vector_t>(k + (k * (k - 1)) / 2),
                                  k, lp);
    } else {
      return cov_matrix_constrain(this->read<vector_t>(k + (k * (k - 1)) / 2),
                                  k);
    }
  }

  template <typename Ret, bool Jacobian, typename LP, typename... Sizes,
            require_std_vector_t<Ret>* = nullptr>
  auto read_cov_matrix(LP& lp, const size_t vecsize, const Sizes&... sizes) {
    using ret_value_t = value_type_t<Ret>;
    std::decay_t<Ret> ret;
    for (size_t i = 0; i < vecsize; ++i) {
      ret.emplace_back(
          this->read_cov_matrix<ret_value_t, Jacobian>(lp, sizes...));
    }
    return ret;
  }
};

}  // namespace io
}  // namespace stan

#endif
