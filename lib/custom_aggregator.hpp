
#ifndef FCPP_WAREHOUSE_CUSTOM_AGGREGATOR_H_
#define FCPP_WAREHOUSE_CUSTOM_AGGREGATOR_H_

#include "lib/fcpp.hpp"

namespace fcpp {

namespace aggregator {

//! @brief Aggregates vectors of values by averaging.
template <typename T, bool only_finite = std::numeric_limits<T>::has_infinity>
class vector_mean {
  public:
    //! @brief The type of values aggregated.
    using type = std::vector<T>;

    //! @brief The type of the aggregation result, given the tag of the aggregated values.
    template <typename U>
    using result_type = common::tagged_tuple_t<vector_mean<U, only_finite>, T>;

    //! @brief Default constructor.
    vector_mean() = default;

    //! @brief Combines aggregated values.
    vector_mean& operator+=(vector_mean const& o) {
        m_sum += o.m_sum;
        m_count += o.m_count;
        return *this;
    }

    //! @brief Erases a value from the aggregation set.
    void erase(std::vector<T>& values) {
        for (T value: values) {
            if (not only_finite or std::isfinite(value)) {
                m_sum -= value;
                m_count--;
            }
        }
    }

    //! @brief Inserts a new value to be aggregated.
    void insert(std::vector<T>& values) {
        for (T value: values) {
            if (not only_finite or std::isfinite(value)) {
                m_sum += value;
                m_count++;
            }
        }
    }

    //! @brief The results of aggregation.
    template <typename U>
    result_type<U> result() const {
        return {m_count == 0 ? std::numeric_limits<T>::quiet_NaN() : m_sum/m_count};
    }

    //! @brief The aggregator name.
    static std::string name() {
        return "vector_mean";
    }

    //! @brief Outputs the aggregator description.
    template <typename O>
    void header(O& os, std::string tag) const {
        os << details::header(tag, name());
    }

    //! @brief Printed results of aggregation.
    template <typename O>
    void output(O& os) const {
        os << std::get<0>(result<void>()) << " ";
    }

  private:
    T m_sum = 0;
    size_t m_count = 0;
};

//! @brief Aggregates vectors of values by taking the maximum (insert-only).
template <typename T, bool only_finite = std::numeric_limits<T>::has_infinity>
class vector_max {
  public:
    //! @brief The type of values aggregated.
    using type = std::vector<T>;

    //! @brief The type of the aggregation result, given the tag of the aggregated values.
    template <typename U>
    using result_type = common::tagged_tuple_t<vector_max<U, only_finite>, T>;

    //! @brief Default constructor.
    vector_max() = default;

    //! @brief Combines aggregated values.
    vector_max& operator+=(vector_max const& o) {
        m_max = std::max(m_max, o.m_max);
        return *this;
    }

    //! @brief Erases a value from the aggregation set (not supported).
    void erase(std::vector<T>) {
        assert(false);
    }

    //! @brief Inserts a new value to be aggregated.
    void insert(std::vector<T> values) {
        for (T value: values) {
            if (not only_finite or std::isfinite(value)) {
                m_max = std::max(m_max, value);
            }
        }
    }

    //! @brief The results of aggregation.
    template <typename U>
    result_type<U> result() const {
        return {m_max};
    }

    //! @brief The aggregator name.
    static std::string name() {
        return "vector_max";
    }

    //! @brief Outputs the aggregator description.
    template <typename O>
    void header(O& os, std::string tag) const {
        os << details::header(tag, name());
    }

    //! @brief Printed results of aggregation.
    template <typename O>
    void output(O& os) const {
        os << std::get<0>(result<void>()) << " ";
    }

  private:
    T m_max = std::numeric_limits<T>::has_infinity ? -std::numeric_limits<T>::infinity() : std::numeric_limits<T>::lowest();
};

}

}

#endif // FCPP_WAREHOUSE_CUSTOM_AGGREGATOR_H_