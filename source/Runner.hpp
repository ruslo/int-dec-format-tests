#ifndef RUNNER_HPP_
#define RUNNER_HPP_

// Copyright (c) 2014, Ruslan Baratov
// All rights reserved.

#include <iomanip> // std::setw
#include <chrono>
#include <cmath> // std::sqrt
#include <numeric> // std::accumulate
#include <cassert>

#include "Input.hpp"
#include "Output.hpp"
#include "Timer.hpp"

template <class Input, class Algo>
class Runner {
 public:
  using Duration = Timer::Duration;

  static const int ALL = 10000000;
#if defined(NDEBUG)
  static const int TIMED_RUN = 10;
#else
  static const int TIMED_RUN = 1;
#endif

  Runner(const Input& input, Output& output, const char* name):
      input_(input),
      output_(output),
#if defined(NDEBUG)
      timer_iterations_(ALL / input.size() / TIMED_RUN),
#else
      timer_iterations_(1),
#endif
      name_(name)
  {
    assert(timer_iterations_ > 0);
  }

  void run() {
#if !defined(NDEBUG)
    output_.clear();
#endif
    for (int i = 0; i < TIMED_RUN; ++i) {
      const Timer::TimePoint start = Timer::now();

      for (int j = 0; j < timer_iterations_; ++j) {
        Algo::run(output_.buffer(), input_.values());

        // prevent compiler optimization
        output_.use_buffer(name_);
      }

      const Timer::TimePoint stop = Timer::now();
      durations_.push_back(stop - start);
    }
  }

  Duration average() const {
    if (durations_.empty()) {
      throw std::runtime_error("No run detected");
    }
    Duration sum = std::accumulate(
        durations_.begin(), durations_.end(), Duration(0)
    );
    return sum / durations_.size();
  }

  void output_result(Duration best) const {
    const Duration avg = average();

    std::string output_name;
    if (avg == best) {
      output_name += "(!)";
    }
    else {
      assert(best < avg);
      auto overhead = 100 * (avg - best).count() / best.count();
      output_name += "(+" + std::to_string(overhead) + "%)";
    }

    output_name += " ";
    output_name += name_;

    double avg_tick = avg.count();
    double sd(0);
    for (auto i: durations_) {
      double x(i.count() - avg_tick);
      sd += x * x;
    }
    sd = sd / durations_.size();
    sd = std::sqrt(sd);
    const double var = (sd / avg_tick) * 100.0;

    uint64_t input_size = input_.values().size();
    uint64_t conv_ops = durations_.size() * timer_iterations_ * input_size;
    using Ms = std::chrono::milliseconds;
    std::cout << std::setw(40) << output_name << " [";
    std::cout << "avg:" << avg.count() << " ";
    std::cout << "var:" << var << "% ";
    std::cout << "ms:" << std::chrono::duration_cast<Ms>(avg).count() << " ";
    std::cout << "runs:" << durations_.size() << " ";
    std::cout << "conv_ops:" << conv_ops;
    std::cout << "]" << std::endl;
  }

 private:
  const Input& input_;
  Output& output_;

  const int timer_iterations_;
  const char* name_;
  std::vector<Duration> durations_;
};

#endif // RUNNER_HPP_