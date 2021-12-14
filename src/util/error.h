#pragma once

#include <string>
#include <string_view>

namespace engine {
    template<typename T>
    struct Ok { 
        T value; 
    };

    template<typename E>
    struct Error { 
        E error; 
    };

    template<typename T, typename E>
    struct Result {
        Result(Ok<T>&& value) 
            : pass(true)
            , val(value.value) 
        { }

        Result(Error<E>&& error) 
            : pass(false)
            , err(error.error) 
        { }

        ~Result() {
            if (pass) { val.~T(); } 
            else { err.~E(); }
        }

        operator bool() const { return pass; }
        bool valid() const { return pass; }

        T value() const { return val; }
        E error() const { return err; }

        T *operator->() { return &val; }

        Error<E> forward() const { return Error<E>{ err }; }

    private:
        bool pass;
        union {
            T val;
            E err;
        };
    };

    template<typename T>
    Ok<T> pass(T value) { return Ok<T>{ value }; }

    template<typename E>
    Error<E> fail(E error) { return Error<E>{ error }; }
}
