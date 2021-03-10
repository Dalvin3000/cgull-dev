# CGull /ˈsiː.ɡʌl/

Promises for C++17.

Inspired by BlueBird.js.

development repo (WIP)

## Use cases

Classic syntax:

```cpp
inline auto uv_fs_open_async = cgull::async{ uv_fs_open };

int main(int argc, char **argv) {
    uv_fs_open_async(uv_default_loop(), &open_req, argv[1], O_RDONLY, 0)
        .then(uv_default_loop(),
            [](uv_fs_t* req)
            {
                // ...
            }
        );

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
```

Some C++ sugar:

```cpp
int main(int argc, char **argv) {
    auto promise = some_async_op();

    promise
        << thread_or_context
        >> [](auto some_async_op_result)
        {
            // ...
            return 123;
        }
        << other_context
        >> [](int previous_result)
        {
            // ...
        }
        // same context
        >> []()
        {
            // ...
        };
}
```

## Refactoring roadmap

Feature | Status
--- | ---
std code style conformance | :x:
async wrapped | :heavy_check_mark: (not all tests written)
full qt handler | :x:
full uv handler | :x:
c++ operators sugar | :x:
