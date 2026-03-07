#pragma once

#include <type_traits>
#include <utility>
#include <variant>

#if __has_include(<expected>) && defined(__cpp_lib_expected) && (__cpp_lib_expected >= 202211L)
#include <expected>

namespace pm {
    template<class T, class E>
    using expected = std::expected<T, E>;

    template<class E>
    using unexpected = std::unexpected<E>;
}
#else
namespace pm {
    template<class E>
    class unexpected {
    public:
        explicit unexpected(E e) : error_(std::move(e)) {
        }

        const E &error() const & { return error_; }
        E &error() & { return error_; }
        E &&error() && { return std::move(error_); }

    private:
        E error_;
    };

    template<class T, class E>
    class expected {
    public:
        expected(const T &v) : v_(v) {
        }

        expected(T &&v) : v_(std::move(v)) {
        }

        expected(unexpected<E> &&u) : v_(std::move(u).error()) {
        }

        expected(const unexpected<E> &u) : v_(u.error()) {
        }

        [[nodiscard]] bool has_value() const noexcept { return std::holds_alternative<T>(v_); }
        explicit operator bool() const noexcept { return has_value(); }

        T &value() & { return std::get<T>(v_); }
        const T &value() const & { return std::get<T>(v_); }
        T &&value() && { return std::get<T>(std::move(v_)); }

        E &error() & { return std::get<E>(v_); }
        const E &error() const & { return std::get<E>(v_); }
        E &&error() && { return std::get<E>(std::move(v_)); }

    private:
        std::variant<T, E> v_;
    };

    template<class E>
    class expected<void, E> {
    public:
        expected() : ok_(true) {
        }

        expected(unexpected<E> &&u) : ok_(false), err_(std::move(u).error()) {
        }

        expected(const unexpected<E> &u) : ok_(false), err_(u.error()) {
        }

        [[nodiscard]] bool has_value() const noexcept { return ok_; }
        explicit operator bool() const noexcept { return ok_; }

        void value() const {
        } // no-op

        E &error() & { return err_; }
        const E &error() const & { return err_; }
        E &&error() && { return std::move(err_); }

    private:
        bool ok_{false};
        E err_{};
    };
} // namespace pm
#endif
