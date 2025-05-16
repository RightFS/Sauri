/**
 * @file result.hpp
 * @brief A template class that encapsulates operation results
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod System Kit.
 * Created: 2025-03-26
 * Author: chenxu
 */

#ifndef LEIGOD_SYSTEM_KIT_CORE_RESULT_HPP
#define LEIGOD_SYSTEM_KIT_CORE_RESULT_HPP

#include "error.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#ifdef WIN32
#include <atlbase.h>
#endif

namespace leigod {
namespace system_kit {

/**
 * @struct Error
 * @brief Represents an error with code and message
 *
 * This structure encapsulates error information that is returned
 * when an operation fails.
 */
struct Error {
    ErrorCode code;       ///< Error code
    std::string message;  ///< Error message

    /**
     * @brief Constructor
     * @param c Error code
     * @param msg Error message
     */
    Error(ErrorCode c, std::string msg) : code(c), message(std::move(msg)) {}

    /**
     * @brief Get string representation of the error
     * @return Error code as string and message
     */
    std::string toString() const {
        return std::string(errorCodeToString(code)) + ": " + message;
    }
};

/**
 * @class ResultException
 * @brief Exception thrown when accessing a failed Result's value
 */
class ResultException : public std::runtime_error {
public:
    /**
     * @brief Constructor
     * @param error The error that occurred
     */
    explicit ResultException(const Error& error)
        : std::runtime_error(error.toString()), m_error(error) {}

    /**
     * @brief Get the error
     * @return The error
     */
    const Error& error() const {
        return m_error;
    }

private:
    Error m_error;
};

/**
 * @class Result
 * @brief Class that represents the result of an operation that might fail
 *
 * Result is a template class used for returning values from functions that
 * can fail. It can either contain a value of type T (success) or an Error
 * (failure).
 *
 * @tparam T The type of value returned on success
 */
template <typename T>
class Result {
public:
    /**
     * @brief Create a success result with a value
     * @param value The success value
     * @return A Result indicating success with the given value
     */
    static Result<T> success(const T& value) {
        return Result<T>(value);
    }

    /**
     * @brief Create a success result with a moved value
     * @param value The success value to move
     * @return A Result indicating success with the moved value
     */
    static Result<T> success(T&& value) {
        return Result<T>(std::move(value));
    }

    /**
     * @brief Create a failure result with an error
     * @param code The error code
     * @param message The error message
     * @return A Result indicating failure with the given error
     */
    static Result<T> failure(ErrorCode code, const std::string& message) {
        return Result<T>(Error(code, message));
    }

    /**
     * @brief Create a failure result with an error
     * @param error The error
     * @return A Result indicating failure with the given error
     */
    static Result<T> failure(const Error& error) {
        return Result<T>(error);
    }

    /**
     * @brief Copy constructor
     * @param other The Result to copy from
     */
    Result(const Result& other) : m_hasValue(other.m_hasValue) {
        if (m_hasValue) {
            new (&m_storage.value) T(other.m_storage.value);
        } else {
            new (&m_storage.error) Error(other.m_storage.error);
        }
    }

    /**
     * @brief Move constructor
     * @param other The Result to move from
     */
    Result(Result&& other) noexcept : m_hasValue(other.m_hasValue) {
        if (m_hasValue) {
            new (&m_storage.value) T(std::move(other.m_storage.value));
        } else {
            new (&m_storage.error) Error(std::move(other.m_storage.error));
        }
    }

    /**
     * @brief Destructor
     */
    ~Result() {
        if (m_hasValue) {
            m_storage.value.~T();
        } else {
            m_storage.error.~Error();
        }
    }

    /**
     * @brief Copy assignment operator
     * @param other The Result to copy from
     * @return Reference to this Result
     */
    Result& operator=(const Result& other) {
        if (this != &other) {
            // Destroy current value or error
            if (m_hasValue) {
                m_storage.value.~T();
            } else {
                m_storage.error.~Error();
            }

            m_hasValue = other.m_hasValue;

            // Copy new value or error
            if (m_hasValue) {
                new (&m_storage.value) T(other.m_storage.value);
            } else {
                new (&m_storage.error) Error(other.m_storage.error);
            }
        }
        return *this;
    }

    /**
     * @brief Move assignment operator
     * @param other The Result to move from
     * @return Reference to this Result
     */
    Result& operator=(Result&& other) noexcept {
        if (this != &other) {
            // Destroy current value or error
            if (m_hasValue) {
                m_storage.value.~T();
            } else {
                m_storage.error.~Error();
            }

            m_hasValue = other.m_hasValue;

            // Move new value or error
            if (m_hasValue) {
                new (&m_storage.value) T(std::move(other.m_storage.value));
            } else {
                new (&m_storage.error) Error(std::move(other.m_storage.error));
            }
        }
        return *this;
    }

    /**
     * @brief Check if the result is a success
     * @return True if success, false if failure
     */
    bool isSuccess() const {
        return m_hasValue;
    }

    /**
     * @brief Check if the result is a failure
     * @return True if failure, false if success
     */
    bool isFailure() const {
        return !m_hasValue;
    }

    /**
     * @brief Get the contained value
     * @return Reference to the contained value
     * @throws ResultException if the result is a failure
     */
    const T& value() const {
        if (!m_hasValue) {
            throw ResultException(m_storage.error);
        }
        return m_storage.value;
    }

    /**
     * @brief Get the contained value
     * @return Reference to the contained value
     * @throws ResultException if the result is a failure
     */
    T& value() {
        if (!m_hasValue) {
            throw ResultException(m_storage.error);
        }
        return m_storage.value;
    }

    /**
     * @brief Get the error
     * @return Reference to the error
     * @throws std::logic_error if the result is a success
     */
    const Error& error() const {
        if (m_hasValue) {
            throw std::logic_error("Cannot get error from successful result");
        }
        return m_storage.error;
    }

    /**
     * @brief Get a value or a default if the result is a failure
     * @param defaultValue The default value to return if the result is a failure
     * @return The contained value if success, or defaultValue if failure
     */
    T valueOr(const T& defaultValue) const {
        return m_hasValue ? m_storage.value : defaultValue;
    }

private:
    /**
     * @brief Constructor for success
     * @param value The success value
     */
    explicit Result(const T& value) : m_hasValue(true) {
        new (&m_storage.value) T(value);
    }

    /**
     * @brief Constructor for success with move
     * @param value The success value to move
     */
    explicit Result(T&& value) : m_hasValue(true) {
        new (&m_storage.value) T(std::move(value));
    }

    /**
     * @brief Constructor for failure
     * @param error The error
     */
    explicit Result(const Error& error) : m_hasValue(false) {
        new (&m_storage.error) Error(error);
    }

    bool m_hasValue;  ///< Flag indicating whether this result contains a value

    // Use anonymous union with placement new/delete to safely handle types with non-trivial
    // destructors
    union Storage {
        T value;      ///< The value (valid if m_hasValue is true)
        Error error;  ///< The error (valid if m_hasValue is false)

        // Default constructor does nothing - we use placement new to initialize members
        Storage() {}
        // No destructor - we manually call destructors in Result's destructor
        ~Storage() {}
    } m_storage;
};

/**
 * @brief Specialization for void return type
 *
 * This specialization allows functions that return nothing on success
 * to also use the Result pattern.
 */
template <>
class Result<void> {
public:
    /**
     * @brief Create a success result
     * @return A Result indicating success
     */
    static Result<void> success() {
        return Result<void>(true);
    }

    /**
     * @brief Create a failure result with an error
     * @param code The error code
     * @param message The error message
     * @return A Result indicating failure with the given error
     */
    static Result<void> failure(ErrorCode code, const std::string& message) {
        return Result<void>(Error(code, message));
    }

    /**
     * @brief Create a failure result with an error
     * @param error The error
     * @return A Result indicating failure with the given error
     */
    static Result<void> failure(const Error& error) {
        return Result<void>(error);
    }

    /**
     * @brief Default constructor for void result (success)
     */
    Result() : m_isSuccess(true) {}

    /**
     * @brief Copy constructor
     * @param other The Result to copy from
     */
    Result(const Result& other) : m_isSuccess(other.m_isSuccess) {
        if (!m_isSuccess) {
            m_error = other.m_error;
        }
    }

    /**
     * @brief Move constructor
     * @param other The Result to move from
     */
    Result(Result&& other) noexcept : m_isSuccess(other.m_isSuccess) {
        if (!m_isSuccess) {
            m_error = std::move(other.m_error);
        }
    }

    /**
     * @brief Copy assignment operator
     * @param other The Result to copy from
     * @return Reference to this Result
     */
    Result& operator=(const Result& other) {
        if (this != &other) {
            m_isSuccess = other.m_isSuccess;
            if (!m_isSuccess) {
                m_error = other.m_error;
            }
        }
        return *this;
    }

    /**
     * @brief Move assignment operator
     * @param other The Result to move from
     * @return Reference to this Result
     */
    Result& operator=(Result&& other) noexcept {
        if (this != &other) {
            m_isSuccess = other.m_isSuccess;
            if (!m_isSuccess) {
                m_error = std::move(other.m_error);
            }
        }
        return *this;
    }

    /**
     * @brief Check if the result is a success
     * @return True if success, false if failure
     */
    bool isSuccess() const {
        return m_isSuccess;
    }

    /**
     * @brief Check if the result is a failure
     * @return True if failure, false if success
     */
    bool isFailure() const {
        return !m_isSuccess;
    }

    /**
     * @brief Get the error
     * @return Reference to the error
     * @throws std::logic_error if the result is a success
     */
    const Error& error() const {
        if (m_isSuccess) {
            throw std::logic_error("Cannot get error from successful result");
        }
        return m_error;
    }

private:
    /**
     * @brief Constructor for success
     * @param success Whether this is a success result
     */
    explicit Result(bool success) : m_isSuccess(success) {}

    /**
     * @brief Constructor for failure
     * @param error The error
     */
    explicit Result(const Error& error) : m_isSuccess(false), m_error(error) {}

    bool m_isSuccess = true;  ///< Flag indicating whether this result is a success

    // For a failure result, m_error contains the error information
    Error m_error{ErrorCode::Unknown, ""};  ///< The error (valid if m_isSuccess is false)
};

#ifdef WIN32
/**
 * @brief Specialization for CComPtr<T>
 *
 * @tparam T COM interface type
 */
template <typename T>
class Result<CComPtr<T>> {
public:
    /**
     * @brief Default constructor deleted
     */
    Result() = delete;

    /**
     * @brief Copy constructor
     *
     * @param other Result to copy
     */
    Result(const Result& other) : success_(other.success_) {
        if (success_) {
            value_ = other.value_;
        } else {
            new (&error_) Error(other.error_);
        }
    }

    /**
     * @brief Move constructor
     *
     * @param other Result to move from
     */
    Result(Result&& other) noexcept : success_(other.success_) {
        if (success_) {
            value_ = std::move(other.value_);
        } else {
            new (&error_) Error(std::move(other.error_));
        }
    }

    /**
     * @brief Destructor
     */
    ~Result() {
        if (success_) {
            // ComPtr has its own destructor, no need to call it explicitly
        } else {
            error_.~Error();
        }
    }

    /**
     * @brief Copy assignment operator
     *
     * @param other Result to copy
     * @return Result& Reference to this
     */
    Result& operator=(const Result& other) {
        if (this != &other) {
            // 清理现有资源
            if (!success_) {
                error_.~Error();
            }

            // 复制新资源
            success_ = other.success_;
            if (success_) {
                value_ = other.value_;
            } else {
                new (&error_) Error(other.error_);
            }
        }
        return *this;
    }

    /**
     * @brief Move assignment operator
     *
     * @param other Result to move from
     * @return Result& Reference to this
     */
    Result& operator=(Result&& other) noexcept {
        if (this != &other) {
            // 清理现有资源
            if (!success_) {
                error_.~Error();
            }

            // 移动新资源
            success_ = other.success_;
            if (success_) {
                value_ = std::move(other.value_);
            } else {
                new (&error_) Error(std::move(other.error_));
            }
        }
        return *this;
    }

    /**
     * @brief Create a success result
     *
     * @param value Success value
     * @return Result Success result
     */
    static Result success(const CComPtr<T>& value) {
        return Result(value);
    }

    /**
     * @brief Create a success result with rvalue
     *
     * @param value Success value (rvalue)
     * @return Result Success result
     */
    static Result success(CComPtr<T>&& value) {
        return Result(std::move(value));
    }

    /**
     * @brief Create a failure result
     *
     * @param code Error code
     * @param message Error message
     * @return Result Failure result
     */
    static Result failure(ErrorCode code, const std::string& message = "") {
        return Result(Error(code, message));
    }

    /**
     * @brief Create a failure result
     *
     * @param error Error object
     * @return Result Failure result
     */
    static Result failure(const Error& error) {
        return Result(error);
    }

    /**
     * @brief Create a failure result with rvalue
     *
     * @param error Error object (rvalue)
     * @return Result Failure result
     */
    static Result failure(Error&& error) {
        return Result(std::move(error));
    }

    /**
     * @brief Check if result is success
     *
     * @return true if success
     * @return false if failure
     */
    bool isSuccess() const {
        return success_;
    }

    /**
     * @brief Check if result is failure
     *
     * @return true if failure
     * @return false if success
     */
    bool isFailure() const {
        return !success_;
    }

    /**
     * @brief Get success value
     *
     * @return CComPtr<T>& Reference to success value
     * @throws std::logic_error if result is failure
     */
    CComPtr<T>& value() {
        if (!success_) {
            throw std::logic_error("Cannot get value from failure result");
        }
        return value_;
    }

    /**
     * @brief Get success value (const)
     *
     * @return const CComPtr<T>& Reference to success value
     * @throws std::logic_error if result is failure
     */
    const CComPtr<T>& value() const {
        if (!success_) {
            throw std::logic_error("Cannot get value from failure result");
        }
        return value_;
    }

    /**
     * @brief Get error
     *
     * @return Error& Reference to error
     * @throws std::logic_error if result is success
     */
    Error& error() {
        if (success_) {
            throw std::logic_error("Cannot get error from success result");
        }
        return error_;
    }

    /**
     * @brief Get error (const)
     *
     * @return const Error& Reference to error
     * @throws std::logic_error if result is success
     */
    const Error& error() const {
        if (success_) {
            throw std::logic_error("Cannot get error from success result");
        }
        return error_;
    }

private:
    /**
     * @brief Construct a success result
     *
     * @param value Success value
     */
    explicit Result(const CComPtr<T>& value) : success_(true), value_(value) {}

    /**
     * @brief Construct a success result with rvalue
     *
     * @param value Success value (rvalue)
     */
    explicit Result(CComPtr<T>&& value) : success_(true), value_(std::move(value)) {}

    /**
     * @brief Construct a failure result
     *
     * @param error Error object
     */
    explicit Result(const Error& error) : success_(false) {
        new (&error_) Error(error);
    }

    /**
     * @brief Construct a failure result with rvalue
     *
     * @param error Error object (rvalue)
     */
    explicit Result(Error&& error) : success_(false) {
        new (&error_) Error(std::move(error));
    }

    bool success_;  ///< Success flag

    // 使用不同的内存布局处理 ComPtr
    union {
        CComPtr<T> value_;  ///< Success value
        Error error_;       ///< Error object
    };
};
#endif

}  // namespace system_kit
}  // namespace leigod

#endif  // LEIGOD_SYSTEM_KIT_CORE_RESULT_HPP