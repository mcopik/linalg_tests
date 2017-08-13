//  Copyright (c) 2016-2017 Marcin Copik
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LINALG_TESTS_GENERATOR_SHAPE_HPP
#define LINALG_TESTS_GENERATOR_SHAPE_HPP

#include <tuple>
#include <stdexcept>

namespace generator { namespace shape {

    struct matrix_size
    {
        uint32_t rows, cols;
    };

    struct band
    {
        static constexpr bool symmetric = false;
        const uint32_t lower_bandwidth;
        const uint32_t upper_bandwidth;

        band(uint32_t lower_, uint32_t upper_):
                lower_bandwidth(lower_),
                upper_bandwidth(upper_)
        {}

        band(const matrix_size & size):
                lower_bandwidth(size.rows - 1),
                upper_bandwidth(size.cols - 1)
        {}

        const band & to_band(const matrix_size &) const
        {
            return *this;
        }
    };

    struct general
    {
        static constexpr bool symmetric = false;

        band to_band(const matrix_size & size) const
        {
            return band(size.rows - 1, size.cols - 1);
        }
    };

    struct self_adjoint
    {
        static constexpr bool symmetric = true;

        band to_band(const matrix_size & size) const
        {
            if(size.rows != size.cols) {
                throw std::runtime_error("Non-square matrix sizes passed to aself-adjoint matrix!");
            }
            return band(size.rows - 1, size.cols - 1);
        }
    };

    struct upper_triangular
    {
        static constexpr bool symmetric = false;

        band to_band(const matrix_size & size) const
        {
            return band(0, size.cols - 1);
        }
    };

    struct lower_triangular
    {
        static constexpr bool symmetric = false;

        band to_band(const matrix_size & size) const
        {
            return band(size.rows - 1, 0);
        }
    };

    struct tridiagonal : public band
    {
        static constexpr bool symmetric = false;

        tridiagonal(): band(1, 1)
        {}

        band to_band(const matrix_size &) const
        {
            return band(1, 1);
        }
    };

    struct diagonal : public band
    {
        static constexpr bool symmetric = true;

        diagonal(): band(0, 0)
        {}

        band to_band(const matrix_size &) const
        {
            return band(0, 0);
        }
    };

    band merge_band(const band & first, const band & second)
    {
        return band(
            std::min(first.lower_bandwidth, second.lower_bandwidth),
            std::min(first.upper_bandwidth, second.upper_bandwidth)
        );
    }

    template<typename T>
    struct is_shape_type : std::false_type {};

    template<>
    struct is_shape_type<band> : std::true_type {};
    template<>
    struct is_shape_type<general> : std::true_type {};
    template<>
    struct is_shape_type<self_adjoint> : std::true_type {};
    template<>
    struct is_shape_type<upper_triangular> : std::true_type {};
    template<>
    struct is_shape_type<lower_triangular> : std::true_type {};
    template<>
    struct is_shape_type<tridiagonal> : std::true_type {};
    template<>
    struct is_shape_type<diagonal> : std::true_type {};

    namespace detail {

        template<typename T1, typename T2>
        struct tuple_cat_result;

        template<typename... T1, typename T2>
        struct tuple_cat_result<std::tuple<T1...>, T2>
        {
            typedef std::tuple<T1..., T2> type;
        };

        // Forward declarations
        template<typename OldTuple, typename Property,
            typename std::enable_if<!is_shape_type<Property>::value, int>::type = 0
        >
        auto from_properties(const matrix_size &, const band & band_type, OldTuple && tuple,
            Property && property)
            -> std::tuple<band, typename tuple_cat_result<OldTuple, Property>::type>;

        template<typename OldTuple, typename Property, typename... Properties,
            typename std::enable_if<!is_shape_type<Property>::value, int>::type = 0
        >
        decltype(auto) from_properties(const matrix_size & size, const band & band_type, OldTuple && tuple,
            Property && property, Properties &&... props);

        // Update band type with a shape type, tuple unchanged
        template<typename OldTuple, typename Property,
            typename std::enable_if<is_shape_type<Property>::value, int>::type = 0
        >
        std::tuple<band, OldTuple> from_properties(const matrix_size & size, const band & band_type,
            OldTuple && tuple, Property && property)
        {
            return std::make_tuple(
                merge_band(band_type, property.to_band(size)),
                tuple
                );
        }

        template<typename OldTuple, typename Property, typename... Properties,
            typename std::enable_if<is_shape_type<Property>::value, int>::type = 0
        >
        decltype(auto) from_properties(const matrix_size & size, const band & band_type,
            OldTuple && tuple, Property && property, Properties &&... props)
        {
            return detail::from_properties(size,
                merge_band(band_type, property.to_band(size)),
                std::forward<OldTuple>(tuple),
                std::forward<Properties>(props)...
                );
        }

        // Add non-shape type to a non-empty tuple
        template<typename OldTuple, typename Property,
            typename std::enable_if<!is_shape_type<Property>::value, int>::type = 0
        >
        auto from_properties(const matrix_size &, const band & band_type, OldTuple && tuple,
            Property && property)
            -> std::tuple<band, typename tuple_cat_result<OldTuple, Property>::type>
        {
            return std::make_tuple(band_type, std::tuple_cat(tuple, std::make_tuple(std::forward<Property>(property))));
        }

        template<typename OldTuple, typename Property, typename... Properties,
            typename std::enable_if<!is_shape_type<Property>::value, int>::type = 0
        >
        decltype(auto) from_properties(const matrix_size & size, const band & band_type, OldTuple && tuple,
            Property && property, Properties &&... props)
        {
            return detail::from_properties(size,
                band_type,
                std::tuple_cat(tuple, std::make_tuple(std::forward<Property>(property))),
                std::forward<Properties>(props)...
                );
        }
    }

    template<typename... Properties>
    decltype(auto) from_properties(const matrix_size & size, Properties &&... props)
    {
        return detail::from_properties(size,
            band(size),
            std::tuple<>{},
            std::forward<Properties>(props)...
            );
    }

}}

#endif
