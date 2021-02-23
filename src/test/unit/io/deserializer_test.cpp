#include <stan/io/deserializer.hpp>
#include <gtest/gtest.h>

TEST(deserializer, eigen_input) {
  Eigen::Matrix<int, -1, 1> theta_i(1);
  Eigen::VectorXd theta(2);
  theta[0] = 1.0;
  theta[1] = 2.0;
  theta_i[0] = 1;
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double x = deserializer.read<double>();
  EXPECT_FLOAT_EQ(1.0, x);
  double y = deserializer.read<double>();
  EXPECT_FLOAT_EQ(2.0, y);
  int z = deserializer.read<int>();
  EXPECT_EQ(1, z);
  EXPECT_EQ(0U, deserializer.available());
}

TEST(deserializer, zeroSizeVecs) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(1.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);

  EXPECT_FLOAT_EQ(1.0, deserializer.read<double>());  // finish available

  EXPECT_EQ(0, deserializer.read<std::vector<double>>(0).size());
  EXPECT_EQ(0, deserializer.read<Eigen::VectorXd>(0).size());
  EXPECT_EQ(0, deserializer.read<Eigen::RowVectorXd>(0).size());
  EXPECT_EQ(0, deserializer.read<Eigen::MatrixXd>(0, 3).size());
  EXPECT_EQ(0, deserializer.read<Eigen::MatrixXd>(3, 0).size());
  EXPECT_EQ(0, deserializer.read<std::vector<std::vector<Eigen::MatrixXd>>>(0, 0, 0, 0).size());
}

TEST(deserializer_scalar, read) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(1.0);
  theta.push_back(2.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double x = deserializer.read<double>();
  EXPECT_FLOAT_EQ(1.0, x);
  double y = deserializer.read<double>();
  EXPECT_FLOAT_EQ(2.0, y);
  EXPECT_EQ(0U, deserializer.available());
}

TEST(deserializer_rowvector, read) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  for (size_t i = 0; i < 100U; ++i)
    theta.push_back(static_cast<double>(i));
  stan::io::deserializer<double> deserializer(theta, theta_i);
  for (size_t i = 0; i < 7U; ++i) {
    double x = deserializer.read<double>();
    EXPECT_FLOAT_EQ(static_cast<double>(i), x);
  }
  Eigen::Matrix<double, 1, Eigen::Dynamic> y = deserializer.read<Eigen::RowVectorXd>(4);
  EXPECT_EQ(4, y.cols());
  EXPECT_EQ(1, y.rows());
  EXPECT_EQ(4, y.size());
  EXPECT_FLOAT_EQ(7.0, y[0]);
  EXPECT_FLOAT_EQ(8.0, y[1]);
  EXPECT_FLOAT_EQ(9.0, y[2]);
  EXPECT_FLOAT_EQ(10.0, y[3]);

  double z = deserializer.read<double>();
  EXPECT_FLOAT_EQ(11.0, z);
}

TEST(deserializer_stdvec, std_vector_vector) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  for (size_t i = 0; i < 100U; ++i)
    theta.push_back(static_cast<double>(i));
  stan::io::deserializer<double> deserializer(theta, theta_i);
  auto y = deserializer.read<std::vector<Eigen::VectorXd>>(5, 4);
  EXPECT_EQ(5, y.size());
  int sentinal = 0;
  for (int i = 0; i < y.size(); ++i) {
    EXPECT_EQ(4, y[i].rows());
    EXPECT_EQ(1, y[i].cols());
    for (int j = 0; j < 4; ++j) {
      EXPECT_FLOAT_EQ(sentinal, y[i].coeff(j));
      sentinal++;
    }
  }
}

TEST(deserializer_stdvec, std_vector_std_vector_matrix) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  for (size_t i = 0; i < 120U; ++i)
    theta.push_back(static_cast<double>(i));
  stan::io::deserializer<double> deserializer(theta, theta_i);
  auto y = deserializer.read<std::vector<std::vector<Eigen::MatrixXd>>>(5, 4, 3, 2);
  EXPECT_EQ(5, y.size());
  int sentinal = 0;
  for (int i = 0; i < y.size(); ++i) {
    EXPECT_EQ(4, y[i].size());
    for (int j = 0; j < 4; ++j) {
      EXPECT_EQ(3, y[i][j].rows());
      EXPECT_EQ(2, y[i][j].cols());
      for (int k = 0; k < 6; ++k) {
        EXPECT_FLOAT_EQ(sentinal, y[i][j].coeff(k));
        sentinal++;
      }
    }
  }
}

// lower bounds


TEST(deserializer_scalar, read_lb) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-1.0);
  theta.push_back(2.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double x = deserializer.read_lb<double>(-2.0);
  EXPECT_FLOAT_EQ(-1.0, x);
  double y = deserializer.read_lb<double>(1.0);
  EXPECT_FLOAT_EQ(2.0, y);
  EXPECT_EQ(0U, deserializer.available());
}
TEST(deserializer_scalar, read_lb_exception) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-1.0);
  theta.push_back(2.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  EXPECT_NO_THROW(deserializer.read_lb<double>(-1.0));
  EXPECT_THROW(deserializer.read_lb<double>(3.0), std::domain_error);
}
TEST(deserializer_scalar, read_lb_constrain) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-2.0);
  theta.push_back(3.0);
  theta.push_back(-1.0);
  theta.push_back(0.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double lp = 0.0;
  EXPECT_FLOAT_EQ(1.0 + exp(-2.0), (deserializer.read_lb_constrain<double, false>(1.0, lp)));
  EXPECT_FLOAT_EQ(5.0 + exp(3.0), (deserializer.read_lb_constrain<double, false>(5.0, lp)));
  EXPECT_FLOAT_EQ(-2.0 + exp(-1.0), (deserializer.read_lb_constrain<double, false>(-2.0, lp)));
  EXPECT_FLOAT_EQ(15.0 + exp(0.0), (deserializer.read_lb_constrain<double, false>(15.0, lp)));
}
TEST(deserializer_scalar, read_lb_constrain_jacobian) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-2.0);
  theta.push_back(3.0);
  theta.push_back(-1.0);
  theta.push_back(0.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double lp = -1.5;
  EXPECT_FLOAT_EQ(1.0 + exp(-2.0), (deserializer.read_lb_constrain<double, true>(1.0, lp)));
  EXPECT_FLOAT_EQ(5.0 + exp(3.0), (deserializer.read_lb_constrain<double, true>(5.0, lp)));
  EXPECT_FLOAT_EQ(-2.0 + exp(-1.0), (deserializer.read_lb_constrain<double, true>(-2.0, lp)));
  EXPECT_FLOAT_EQ(15.0 + exp(0.0), (deserializer.read_lb_constrain<double, true>(15.0, lp)));
  EXPECT_FLOAT_EQ(-1.5 - 2.0 + 3.0 - 1.0, lp);
}

// ub

TEST(deserializer_scalar, read_ub) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-1.0);
  theta.push_back(2.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double x = deserializer.read_ub<double>(-0.5);
  EXPECT_FLOAT_EQ(-1.0, x);
  double y = deserializer.read_ub<double>(5.0);
  EXPECT_FLOAT_EQ(2.0, y);

  EXPECT_EQ(0U, deserializer.available());
}
TEST(deserializer_scalar, read_ub_exception) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-1.0);
  theta.push_back(2.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  EXPECT_NO_THROW(deserializer.read_ub<double>(-1.0));
  EXPECT_THROW(deserializer.read_ub<double>(1.0), std::domain_error);
}
TEST(deserializer_scalar, read_ub_constrain) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-2.0);
  theta.push_back(3.0);
  theta.push_back(-1.0);
  theta.push_back(0.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double lp = 0;
  EXPECT_FLOAT_EQ(1.0 - exp(-2.0), (deserializer.read_ub_constrain<double, false>(1.0, lp)));
  EXPECT_FLOAT_EQ(5.0 - exp(3.0), (deserializer.read_ub_constrain<double, false>(5.0, lp)));
  EXPECT_FLOAT_EQ(-2.0 - exp(-1.0), (deserializer.read_ub_constrain<double, false>(-2.0, lp)));
  EXPECT_FLOAT_EQ(15.0 - exp(0.0), (deserializer.read_ub_constrain<double, false>(15.0, lp)));
}
TEST(deserializer_scalar, read_ub_constrain_jacobian) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-2.0);
  theta.push_back(3.0);
  theta.push_back(-1.0);
  theta.push_back(0.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double lp = -12.9;
  EXPECT_FLOAT_EQ(1.0 - exp(-2.0), (deserializer.read_ub_constrain<double, true>(1.0, lp)));
  EXPECT_FLOAT_EQ(5.0 - exp(3.0), (deserializer.read_ub_constrain<double, true>(5.0, lp)));
  EXPECT_FLOAT_EQ(-2.0 - exp(-1.0), (deserializer.read_ub_constrain<double, true>(-2.0, lp)));
  EXPECT_FLOAT_EQ(15.0 - exp(0.0), (deserializer.read_ub_constrain<double, true>(15.0, lp)));
  EXPECT_FLOAT_EQ(-12.9 - 2.0 + 3.0 - 1.0, lp);
}

// lub


TEST(deserializer_scalar, read_lub) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-1.0);
  theta.push_back(2.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double x = deserializer.read_lub<double>(-3.0, 3.0);
  EXPECT_FLOAT_EQ(-1.0, x);
  double y = deserializer.read_lub<double>(-3.0, 3.0);
  EXPECT_FLOAT_EQ(2.0, y);

  EXPECT_EQ(0U, deserializer.available());
}
TEST(deserializer_scalar, read_lub_exception) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-1.0);
  theta.push_back(2.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  EXPECT_NO_THROW(deserializer.read_lub<double>(-2.0, 2.0));
  EXPECT_THROW(deserializer.read_lub<double>(-1.0, 1.0), std::domain_error);
}
const double inv_logit_m2 = 0.1192029;  // stan::math::inv_logit(-2.0)
const double inv_logit_m1 = 0.2689414;  // stan::math::inv_logit(-1.0)
const double inv_logit_0 = 0.5;         // stan::math::inv_logit(0)
const double inv_logit_3 = 0.9525741;   // stan::math::inv_logit(3.0)

TEST(deserializer_scalar, read_lub_constrain) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-2.0);
  theta.push_back(3.0);
  theta.push_back(-1.0);
  theta.push_back(0.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double lp = 0;
  EXPECT_FLOAT_EQ(inv_logit_m2, (deserializer.read_lub_constrain<double, false>(0.0, 1.0, lp)));
  EXPECT_FLOAT_EQ(3.0 + 2.0 * inv_logit_3,
                  (deserializer.read_lub_constrain<double, false>(3.0, 5.0, lp)));
  EXPECT_FLOAT_EQ(-3.0 + 5.0 * inv_logit_m1,
                  (deserializer.read_lub_constrain<double, false>(-3.0, 2.0, lp)));
  EXPECT_FLOAT_EQ(-15.0 + 30.0 * inv_logit_0,
                  (deserializer.read_lub_constrain<double, false>(-15.0, 15.0, lp)));
}
TEST(deserializer_scalar, read_lub_constrain_jacobian) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-2.0);
  theta.push_back(3.0);
  theta.push_back(-1.0);
  theta.push_back(0.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double lp = -7.2;
  EXPECT_FLOAT_EQ(0.0 + 1.0 * inv_logit_m2,
                  (deserializer.read_lub_constrain<double, true>(0.0, 1.0, lp)));
  EXPECT_FLOAT_EQ(3.0 + 2.0 * inv_logit_3,
                  (deserializer.read_lub_constrain<double, true>(3.0, 5.0, lp)));
  EXPECT_FLOAT_EQ(-3.0 + 5.0 * inv_logit_m1,
                  (deserializer.read_lub_constrain<double, true>(-3.0, 2.0, lp)));
  EXPECT_FLOAT_EQ(-15.0 + 30.0 * inv_logit_0,
                  (deserializer.read_lub_constrain<double, true>(-15.0, 15.0, lp)));
  double expected_lp = -7.2
                       + log((1.0 - 0.0) * inv_logit_m2 * (1 - inv_logit_m2))
                       + log((5.0 - 3.0) * inv_logit_3 * (1 - inv_logit_3))
                       + log((2.0 - -3.0) * inv_logit_m1 * (1 - inv_logit_m1))
                       + log((15.0 - -15.0) * inv_logit_0 * (1 - inv_logit_0));
  EXPECT_FLOAT_EQ(expected_lp, lp);
}

// offset multiplier

TEST(deserializer_scalar, read_offset_multiplier) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-1.0);
  theta.push_back(2.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double x = deserializer.read_offset_multiplier<double>(-3.0, 3.0);
  EXPECT_FLOAT_EQ(-1.0, x);
  double y = deserializer.read_offset_multiplier<double>(-3.0, 3.0);
  EXPECT_FLOAT_EQ(2.0, y);

  EXPECT_EQ(0U, deserializer.available());
}
TEST(deserializer_scalar, offset_multiplier_exception) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-1.0);
  theta.push_back(2.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double lp = 0;
  EXPECT_NO_THROW(deserializer.read_offset_multiplier<double>(-2.0, -2.0));
  EXPECT_THROW((deserializer.read_offset_multiplier<double, false>(-2.0, -2.0, lp)),
               std::domain_error);
}

TEST(deserializer_scalar, offset_multiplier_constrain) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-2.0);
  theta.push_back(3.0);
  theta.push_back(-1.0);
  theta.push_back(0.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double lp = 0;
  EXPECT_FLOAT_EQ(-2.0, (deserializer.read_offset_multiplier<double, false>(0.0, 1.0, lp)));
  EXPECT_FLOAT_EQ(3.0 + 5.0 * 3.0,
                  (deserializer.read_offset_multiplier<double, false>(3.0, 5.0, lp)));
  EXPECT_FLOAT_EQ(-3.0 + 2.0 * -1.0,
                  (deserializer.read_offset_multiplier<double, false>(-3.0, 2.0, lp)));
  EXPECT_FLOAT_EQ(-15.0,
                  (deserializer.read_offset_multiplier<double, false>(-15.0, 15.0, lp)));
}
TEST(deserializer_scalar, offset_multiplier_constrain_jacobian) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-2.0);
  theta.push_back(3.0);
  theta.push_back(-1.0);
  theta.push_back(0.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double lp = -7.2;
  EXPECT_FLOAT_EQ(-2.0,
                  (deserializer.read_offset_multiplier<double, true>(0.0, 1.0, lp)));
  EXPECT_FLOAT_EQ(3.0 + 5.0 * 3.0,
                  (deserializer.read_offset_multiplier<double, true>(3.0, 5.0, lp)));
  EXPECT_FLOAT_EQ(-3.0 + 2.0 * -1.0,
                  (deserializer.read_offset_multiplier<double, true>(-3.0, 2.0, lp)));
  EXPECT_FLOAT_EQ(-15.0,
                  (deserializer.read_offset_multiplier<double, true>(-15.0, 15.0, lp)));
  double expected_lp = -7.2 + log(1.0) + log(5.0) + log(2.0) + log(15.0);
  EXPECT_FLOAT_EQ(expected_lp, lp);
}

// prob

TEST(deserializer_scalar, prob) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(0.9);
  theta.push_back(0.1);
  theta.push_back(0.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double p1 = deserializer.read_prob<double>();
  EXPECT_FLOAT_EQ(0.9, p1);
  double p2 = deserializer.read_prob<double>();
  EXPECT_FLOAT_EQ(0.1, p2);
  double p3 = deserializer.read_prob<double>();
  EXPECT_FLOAT_EQ(0.0, p3);

  EXPECT_EQ(0U, deserializer.available());
}
TEST(deserializer_scalar, prob_constrain) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-2.0);
  theta.push_back(3.0);
  theta.push_back(-1.0);
  theta.push_back(0.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double lp = 0;
  EXPECT_FLOAT_EQ(inv_logit_m2, (deserializer.read_prob<double, false>(lp)));
  EXPECT_FLOAT_EQ(inv_logit_3, (deserializer.read_prob<double, false>(lp)));
  EXPECT_FLOAT_EQ(inv_logit_m1, (deserializer.read_prob<double, false>(lp)));
  EXPECT_FLOAT_EQ(inv_logit_0, (deserializer.read_prob<double, false>(lp)));
}
TEST(deserializer_scalar, prob_constrain_jacobian) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-2.0);
  theta.push_back(3.0);
  theta.push_back(-1.0);
  theta.push_back(0.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double lp = -0.88;
  EXPECT_FLOAT_EQ(inv_logit_m2, (deserializer.read_prob<double, true>(lp)));
  EXPECT_FLOAT_EQ(inv_logit_3, (deserializer.read_prob<double, true>(lp)));
  EXPECT_FLOAT_EQ(inv_logit_m1, (deserializer.read_prob<double, true>(lp)));
  EXPECT_FLOAT_EQ(inv_logit_0, (deserializer.read_prob<double, true>(lp)));
  double expected_lp = -0.88 + log(inv_logit_m2 * (1.0 - inv_logit_m2))
                       + log(inv_logit_3 * (1.0 - inv_logit_3))
                       + log(inv_logit_m1 * (1.0 - inv_logit_m1))
                       + log(inv_logit_0 * (1.0 - inv_logit_0));
  EXPECT_FLOAT_EQ(expected_lp, lp);
}

// corr

TEST(deserializer_scalar, corr) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-0.9);
  theta.push_back(0.1);
  theta.push_back(0.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double rho1 = deserializer.read_corr<double>();
  EXPECT_FLOAT_EQ(-0.9, rho1);
  double rho2 = deserializer.read_corr<double>();
  EXPECT_FLOAT_EQ(0.1, rho2);
  double rho3 = deserializer.read_corr<double>();
  EXPECT_FLOAT_EQ(0.0, rho3);

  EXPECT_EQ(0U, deserializer.available());
}
TEST(deserializer_scalar, corr_exception) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-0.9);
  theta.push_back(-1.1);
  theta.push_back(1.1);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  EXPECT_NO_THROW(deserializer.read_corr<double>());
  EXPECT_THROW(deserializer.read_corr<double>(), std::domain_error);
  EXPECT_THROW(deserializer.read_corr<double>(), std::domain_error);
}
TEST(deserializer_scalar, corr_constrain) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-2.0);
  theta.push_back(3.0);
  theta.push_back(-1.0);
  theta.push_back(0.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double lp = 0;
  EXPECT_FLOAT_EQ(tanh(-2.0), (deserializer.read_corr<double, false>(lp)));
  EXPECT_FLOAT_EQ(tanh(3.0), (deserializer.read_corr<double, false>(lp)));
  EXPECT_FLOAT_EQ(tanh(-1.0), (deserializer.read_corr<double, false>(lp)));
  EXPECT_FLOAT_EQ(tanh(0.0), (deserializer.read_corr<double, false>(lp)));
}
TEST(deserializer_scalar, corr_constrain_jacobian) {
  using std::tanh;
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-2.0);
  theta.push_back(3.0);
  theta.push_back(-1.0);
  theta.push_back(0.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double lp = -10.0;
  EXPECT_FLOAT_EQ(tanh(-2.0), (deserializer.read_corr<double, true>(lp)));
  EXPECT_FLOAT_EQ(tanh(3.0), (deserializer.read_corr<double, true>(lp)));
  EXPECT_FLOAT_EQ(tanh(-1.0), (deserializer.read_corr<double, true>(lp)));
  EXPECT_FLOAT_EQ(tanh(0.0), (deserializer.read_corr<double, true>(lp)));
  double expected_lp = -10.0 + log(1.0 - tanh(-2.0) * tanh(-2.0))
                       + log(1.0 - tanh(3.0) * tanh(3.0))
                       + log(1.0 - tanh(-1.0) * tanh(-1.0))
                       + log(1.0 - tanh(0.0) * tanh(0.0));
  EXPECT_FLOAT_EQ(expected_lp, lp);
}

// vectors

TEST(deserializer_vector, read) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  for (size_t i = 0; i < 100U; ++i)
    theta.push_back(static_cast<double>(i));
  stan::io::deserializer<double> deserializer(theta, theta_i);
  for (size_t i = 0; i < 7U; ++i) {
    double x = deserializer.read<double>();
    EXPECT_FLOAT_EQ(static_cast<double>(i), x);
  }
  Eigen::Matrix<double, Eigen::Dynamic, 1> y = deserializer.read<Eigen::VectorXd>(4);
  EXPECT_EQ(4, y.rows());
  EXPECT_EQ(1, y.cols());
  EXPECT_EQ(4, y.size());
  EXPECT_FLOAT_EQ(7.0, y[0]);
  EXPECT_FLOAT_EQ(8.0, y[1]);
  EXPECT_FLOAT_EQ(9.0, y[2]);
  EXPECT_FLOAT_EQ(10.0, y[3]);

  double z = deserializer.read<double>();
  EXPECT_FLOAT_EQ(11.0, z);
}

TEST(deserializer_vector, unit_vector) {
  std::vector<int> theta_i(0);
  std::vector<double> theta(4, sqrt(0.25));
  stan::io::deserializer<double> deserializer(theta, theta_i);
  Eigen::VectorXd y = deserializer.read_unit_vector<Eigen::VectorXd>(4);
  EXPECT_EQ(4, y.size());
  EXPECT_FLOAT_EQ(sqrt(0.25), y[0]);
  EXPECT_FLOAT_EQ(sqrt(0.25), y[1]);
  EXPECT_FLOAT_EQ(sqrt(0.25), y[2]);
  EXPECT_FLOAT_EQ(sqrt(0.25), y[3]);
}

TEST(deserializer_vector, unit_vector_exception) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  for (int i = 0; i < 100; ++i)
    theta.push_back(static_cast<double>(i));
  stan::io::deserializer<double> deserializer(theta, theta_i);
  theta[0] = 0.00;
  theta[1] = -sqrt(0.29);
  theta[2] = sqrt(0.70);
  theta[3] = -sqrt(0.01);
  theta[4] = sqrt(1.0);
  theta[5] = sqrt(1.0);
  EXPECT_NO_THROW(deserializer.read_unit_vector<Eigen::VectorXd>(4));
  EXPECT_THROW(deserializer.read_unit_vector<Eigen::VectorXd>(2), std::domain_error);
  EXPECT_THROW(deserializer.read_unit_vector<Eigen::VectorXd>(0), std::invalid_argument);
}

TEST(deserializer_vector, simplex) {
  std::vector<int> theta_i(0);
  std::vector<double> theta(4, 0.25);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  Eigen::Matrix<double, Eigen::Dynamic, 1> y = deserializer.read_simplex<Eigen::VectorXd>(4);
  EXPECT_EQ(4, y.size());
  EXPECT_FLOAT_EQ(0.25, y[0]);
  EXPECT_FLOAT_EQ(0.25, y[1]);
  EXPECT_FLOAT_EQ(0.25, y[2]);
  EXPECT_FLOAT_EQ(0.25, y[3]);
}
TEST(deserializer_vector, simplex_exception) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  for (int i = 0; i < 100; ++i)
    theta.push_back(static_cast<double>(i));
  stan::io::deserializer<double> deserializer(theta, theta_i);
  theta[0] = 0.00;
  theta[1] = 0.29;
  theta[2] = 0.70;
  theta[3] = 0.01;
  theta[4] = 1.0;
  theta[5] = 1.0;
  EXPECT_NO_THROW(deserializer.read_simplex<Eigen::VectorXd>(4));
  EXPECT_THROW(deserializer.read_simplex<Eigen::VectorXd>(2), std::domain_error);
  EXPECT_THROW(deserializer.read_simplex<Eigen::VectorXd>(0), std::invalid_argument);
}

TEST(deserializer_vector, ordered) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  for (int i = 0; i < 100.0; ++i)
    theta.push_back(static_cast<double>(i));
  stan::io::deserializer<double> deserializer(theta, theta_i);
  EXPECT_FLOAT_EQ(0.0, deserializer.read<double>());  // throw away theta[0]
  Eigen::Matrix<double, Eigen::Dynamic, 1> y = deserializer.read_ordered<Eigen::VectorXd>(5);
  EXPECT_EQ(5, y.size());
  EXPECT_FLOAT_EQ(1.0, y[0]);
  EXPECT_FLOAT_EQ(2.0, y[1]);
  EXPECT_FLOAT_EQ(5.0, y[4]);
  EXPECT_FLOAT_EQ(6.0, deserializer.read<double>());
}
TEST(deserializer_vector, ordered_exception) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  for (int i = 0; i < 100.0; ++i)
    theta.push_back(static_cast<double>(i));
  stan::io::deserializer<double> deserializer(theta, theta_i);
  EXPECT_FLOAT_EQ(0.0, deserializer.read<double>());  // throw away theta[0]
  Eigen::Matrix<double, Eigen::Dynamic, 1> y = deserializer.read_ordered<Eigen::VectorXd>(5);
  EXPECT_EQ(5, y.size());
  EXPECT_FLOAT_EQ(1.0, y[0]);
  EXPECT_FLOAT_EQ(2.0, y[1]);
  EXPECT_FLOAT_EQ(5.0, y[4]);
  EXPECT_FLOAT_EQ(6.0, deserializer.read<double>());
}
TEST(deserializer_vector, ordered_constrain) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(3.0);
  theta.push_back(-1.0);
  theta.push_back(-2.0);
  theta.push_back(0.0);
  double v0 = 3.0;
  double v1 = v0 + exp(-1.0);
  double v2 = v1 + exp(-2.0);
  double v3 = v2 + exp(0.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double lp = 0;
  Eigen::Matrix<double, Eigen::Dynamic, 1> phi(deserializer.read_ordered<Eigen::VectorXd, false>(lp, 4));
  EXPECT_FLOAT_EQ(v0, phi[0]);
  EXPECT_FLOAT_EQ(v1, phi[1]);
  EXPECT_FLOAT_EQ(v2, phi[2]);
  EXPECT_FLOAT_EQ(v3, phi[3]);
}
TEST(deserializer_vector, ordered_constrain_jacobian) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(3.0);
  theta.push_back(-1.0);
  theta.push_back(-2.0);
  theta.push_back(0.0);
  double v0 = 3.0;
  double v1 = v0 + exp(-1.0);
  double v2 = v1 + exp(-2.0);
  double v3 = v2 + exp(0.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double lp = -101.1;
  double expected_lp = lp - 1.0 - 2.0 + 0.0;
  Eigen::Matrix<double, Eigen::Dynamic, 1> phi(deserializer.read_ordered<Eigen::VectorXd, true>(lp, 4));
  EXPECT_FLOAT_EQ(v0, phi[0]);
  EXPECT_FLOAT_EQ(v1, phi[1]);
  EXPECT_FLOAT_EQ(v2, phi[2]);
  EXPECT_FLOAT_EQ(v3, phi[3]);
  EXPECT_FLOAT_EQ(expected_lp, lp);
}

TEST(deserializer_vector, positive_ordered) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  for (int i = 0; i < 100.0; ++i)
    theta.push_back(static_cast<double>(i));
  stan::io::deserializer<double> deserializer(theta, theta_i);
  EXPECT_FLOAT_EQ(0.0, deserializer.read<double>());  // throw away theta[0]
  Eigen::Matrix<double, Eigen::Dynamic, 1> y = deserializer.read_positive_ordered<Eigen::VectorXd>(5);
  EXPECT_EQ(5, y.size());
  EXPECT_FLOAT_EQ(1.0, y[0]);
  EXPECT_FLOAT_EQ(2.0, y[1]);
  EXPECT_FLOAT_EQ(5.0, y[4]);
  EXPECT_FLOAT_EQ(6.0, deserializer.read<double>());
}

TEST(deserializer_vector, positive_ordered_constrain) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(3.0);
  theta.push_back(-1.0);
  theta.push_back(-2.0);
  theta.push_back(0.0);
  double v0 = exp(3.0);
  double v1 = v0 + exp(-1.0);
  double v2 = v1 + exp(-2.0);
  double v3 = v2 + exp(0.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double lp = 0;
  Eigen::Matrix<double, Eigen::Dynamic, 1> phi(
      deserializer.read_positive_ordered<Eigen::VectorXd, false>(lp, 4));
  EXPECT_FLOAT_EQ(v0, phi[0]);
  EXPECT_FLOAT_EQ(v1, phi[1]);
  EXPECT_FLOAT_EQ(v2, phi[2]);
  EXPECT_FLOAT_EQ(v3, phi[3]);
}

TEST(deserializer_vector, positive_ordered_constrain_jacobian) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(3.0);
  theta.push_back(-1.0);
  theta.push_back(-2.0);
  theta.push_back(0.0);
  double v0 = exp(3.0);
  double v1 = v0 + exp(-1.0);
  double v2 = v1 + exp(-2.0);
  double v3 = v2 + exp(0.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double lp = -101.1;
  double expected_lp = lp + 3.0 - 1.0 - 2.0 + 0.0;
  Eigen::Matrix<double, Eigen::Dynamic, 1> phi(
      deserializer.read_positive_ordered<Eigen::VectorXd, true>(lp, 4));
  EXPECT_FLOAT_EQ(v0, phi[0]);
  EXPECT_FLOAT_EQ(v1, phi[1]);
  EXPECT_FLOAT_EQ(v2, phi[2]);
  EXPECT_FLOAT_EQ(v3, phi[3]);
  EXPECT_FLOAT_EQ(expected_lp, lp);
}

// matrix

TEST(deserializer_matrix, read) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  for (size_t i = 0; i < 100.0; ++i)
    theta.push_back(static_cast<double>(i));
  stan::io::deserializer<double> deserializer(theta, theta_i);
  for (int i = 0; i < 7; ++i) {
    double x = deserializer.read<double>();
    EXPECT_FLOAT_EQ(static_cast<double>(i), x);
  }
  using eig_mat = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>;
  eig_mat y = deserializer.read<eig_mat>(3, 2);
  EXPECT_EQ(3, y.rows());
  EXPECT_EQ(2, y.cols());
  EXPECT_FLOAT_EQ(7.0, y(0, 0));
  EXPECT_FLOAT_EQ(8.0, y(1, 0));
  EXPECT_FLOAT_EQ(9.0, y(2, 0));
  EXPECT_FLOAT_EQ(10.0, y(0, 1));
  EXPECT_FLOAT_EQ(11.0, y(1, 1));
  EXPECT_FLOAT_EQ(12.0, y(2, 1));

  double a = deserializer.read<double>();
  EXPECT_FLOAT_EQ(13.0, a);
}


TEST(deserializer_matrix, corr_matrix) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  for (int i = 0; i < 100.0; ++i)
    theta.push_back(static_cast<double>(i));
  theta[0] = 1.0000000000000000;
  theta[1] = 0.1817621852191252;
  theta[2] = 0.8620926037637362;
  theta[3] = 0.1817621852191252;
  theta[4] = 1.0000000000000000;
  theta[5] = 0.2248293054822660;
  theta[6] = 0.8620926037637362;
  theta[7] = 0.2248293054822660;
  theta[8] = 1.0000000000000001;  // allow some tolerance
  stan::io::deserializer<double> deserializer(theta, theta_i);
  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> S
      = deserializer.read_corr<Eigen::MatrixXd>(3);
  EXPECT_FLOAT_EQ(theta[0], S(0, 0));
}
TEST(deserializer_matrix, corr_matrix_exception) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  for (int i = 0; i < 100.0; ++i)
    theta.push_back(static_cast<double>(i));
  theta[0] = 1.5;
  theta[1] = 1.0;
  theta[2] = 2.0;
  theta[3] = 0.0;
  theta[4] = 1.0;
  stan::io::deserializer<double> deserializer(theta, theta_i);
  EXPECT_THROW(deserializer.read_corr<Eigen::MatrixXd>(1), std::domain_error);
  EXPECT_THROW(deserializer.read_corr<Eigen::MatrixXd>(2), std::domain_error);
}
TEST(deserializer_matrix, corr_matrix_constrain) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-2.0);
  theta.push_back(-1.0);
  theta.push_back(0.0);
  theta.push_back(0.5);
  theta.push_back(1.0);
  theta.push_back(2.0);
  theta.push_back(1.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double lp = 0;
  Matrix<double, Dynamic, Dynamic> R(deserializer.read_corr<Eigen::MatrixXd, false>(lp, 3U));
  EXPECT_EQ(3, R.rows());
  EXPECT_EQ(3, R.cols());
  EXPECT_EQ(9, R.size());
  EXPECT_EQ(4U, deserializer.available());
  for (size_t i = 0; i < 3U; ++i) {
    EXPECT_FLOAT_EQ(1.0, R(i, i));
    for (size_t j = i + 1; j < 3U; ++j)
      EXPECT_FLOAT_EQ(R(i, j), R(j, i));
  }
  Eigen::SelfAdjointEigenSolver<
      Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>>
      solver(R, Eigen::EigenvaluesOnly);
  assert(solver.eigenvalues()[0] > 1E-10);
}
TEST(deserializer_matrix, corr_matrix_constrain_jacobian) {
  std::vector<int> theta_i;
  std::vector<double> theta;
  theta.push_back(-2.0);
  theta.push_back(-1.0);
  theta.push_back(0.0);
  theta.push_back(0.5);
  theta.push_back(1.0);
  theta.push_back(2.0);
  theta.push_back(1.0);
  stan::io::deserializer<double> deserializer(theta, theta_i);
  double lp = -9.2;
  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> R(
      deserializer.read_corr<Eigen::MatrixXd, true>(lp, 3U));
  EXPECT_EQ(3, R.rows());
  EXPECT_EQ(3, R.cols());
  EXPECT_EQ(9, R.size());
  EXPECT_EQ(4U, deserializer.available());
  for (size_t i = 0; i < 3U; ++i) {
    EXPECT_FLOAT_EQ(1.0, R(i, i));
    for (size_t j = i + 1; j < 3U; ++j)
      EXPECT_FLOAT_EQ(R(i, j), R(j, i));
  }
  Eigen::SelfAdjointEigenSolver<
      Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>>
      solver(R, Eigen::EigenvaluesOnly);
  assert(solver.eigenvalues()[0] > 1E-10);
  // FIXME: test jacobian
}

// var matrix


TEST(deserializer_varmat, var_vector) {
  using stan::math::var;
  using stan::math::var_value;
  using var_vec = var_value<Eigen::VectorXd>;
  std::vector<var> theta{0, 1, 2, 3, 4};
  std::vector<int> theta_i;
  stan::io::deserializer<stan::math::var> deserializer(theta, theta_i);
  auto vec_x = deserializer.read<var_vec>(5);
  EXPECT_TRUE((stan::is_var_vector<decltype(vec_x)>::value));
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(vec_x.val()(i), i);
  }
  auto vec_x_empty = deserializer.read<var_vec>(0);
  stan::math::recover_memory();
}

TEST(deserializer_varmat, var_vector_double) {
  using stan::math::var;
  using stan::math::var_value;
  using var_vec = var_value<Eigen::VectorXd>;
  std::vector<double> theta{0, 1, 2, 3, 4};
  std::vector<int> theta_i;
  stan::io::deserializer<double> deserializer(theta, theta_i);
  auto vec_x = deserializer.read<var_vec>(5);
  EXPECT_TRUE(
      (stan::is_eigen_vector<decltype(vec_x)>::value
       && std::is_arithmetic<stan::value_type_t<decltype(vec_x)>>::value));
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(vec_x.val()(i), i);
  }
}

TEST(deserializer_varmat, var_row_vector) {
  using stan::math::var;
  using stan::math::var_value;
  using var_row_vec = var_value<Eigen::RowVectorXd>;
  std::vector<var> theta{0, 1, 2, 3, 4};
  std::vector<int> theta_i;
  stan::io::deserializer<stan::math::var> deserializer(theta, theta_i);
  auto vec_x = deserializer.read<var_row_vec>(5);
  EXPECT_TRUE((stan::is_var_row_vector<decltype(vec_x)>::value));
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(vec_x.val()(i), i);
  }
  auto vec_x_empty = deserializer.read<var_row_vec>(0);
}

TEST(deserializer_varmat, var_row_vector_double) {
  using stan::math::var;
  using stan::math::var_value;
  using var_row_vec = var_value<Eigen::RowVectorXd>;
  std::vector<double> theta{0, 1, 2, 3, 4};
  std::vector<int> theta_i;
  stan::io::deserializer<double> deserializer(theta, theta_i);
  auto vec_x = deserializer.read<var_row_vec>(5);
  EXPECT_TRUE(
      (stan::is_eigen_row_vector<decltype(vec_x)>::value
       && std::is_arithmetic<stan::value_type_t<decltype(vec_x)>>::value));
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(vec_x.val()(i), i);
  }
  auto vec_x_empty = deserializer.read<var_row_vec>(0);
}

TEST(deserializer_varmat, var_matrix) {
  using stan::math::var;
  using stan::math::var_value;
  using var_matrix = var_value<Eigen::MatrixXd>;
  std::vector<var> theta{0, 1, 2, 3, 4, 5, 6, 7, 8};
  std::vector<int> theta_i;
  stan::io::deserializer<stan::math::var> deserializer(theta, theta_i);
  auto mat_x = deserializer.read<var_matrix>(3, 3);
  EXPECT_TRUE((stan::is_var_matrix<decltype(mat_x)>::value));
  for (int i = 0; i < 9; ++i) {
    EXPECT_EQ(mat_x.val()(i), i);
  }
  auto mat_x_empty = deserializer.read<var_matrix>(0, 0);
}

TEST(deserializer_varmat, var_matrix_double) {
  using stan::math::var;
  using stan::math::var_value;
  using var_matrix = var_value<Eigen::MatrixXd>;
  std::vector<double> theta{0, 1, 2, 3, 4, 5, 6, 7, 8};
  std::vector<int> theta_i;
  stan::io::deserializer<double> deserializer(theta, theta_i);
  auto mat_x = deserializer.read<var_matrix>(3, 3);
  EXPECT_TRUE(
      (stan::is_eigen_dense_dynamic<decltype(mat_x)>::value
       && std::is_arithmetic<stan::value_type_t<decltype(mat_x)>>::value));
  for (int i = 0; i < 9; ++i) {
    EXPECT_EQ(mat_x.val()(i), i);
  }
  auto mat_x_empty = deserializer.read<var_matrix>(0, 0);
}
