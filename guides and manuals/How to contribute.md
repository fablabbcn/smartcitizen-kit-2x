## Coding Guidelines
To ensure the software is robust and well-suited for embedded/IoT applications, **please follow these guidelines for every contribution**:

1. **Avoid** `virtual` classes. **Use composition over inheritance**
2. **Avoid standard types** inside the code. **Use type aliases** and define them in a dedicated header.
3. **Replace chip-specific** functions, variables, etc. **with abstraction/aliases** and define them in a dedicated header.
	- ❔ The header will be used as a driver, so different chips can be used only changing the header in compilation.
4. **Use RAII** - constructors/destructors for memory safety
	- RAII = Resource Acquisition Is Initialization

5. Each **function handles one** task
6. **Split logic** into reusable components

7. **Leverage constant expressions** - `constexpr`
	- Use `constexpr` with templates for compile-time optimization
	- Use `constexpr` where possible, specifically for constants and expressions that don't require outer input
8. **Leverage non-throwing exceptions** - `noexcpet`
    - Use `noexcept` for functions that won't throw exceptions
    - Mark entire function blocks with `noexcept` where applicable
    - ❔ It reduces runtime overhead, enables aggressive compiler optimization, and guarantees predictable execution
9. Make sure the processor and all other **components work the minimal required time** and enter idle state when don't work

10. **Avoid dynamic** allocation (`new`/`delete`). **Use static allocation** instead. If dynamic allocation is still needed, _use "custom memory pools" or "fixed buffers"_ instead of traditional "heap allocation"
11. **Avoid STL** - aka `atd::array`, etc
12. **Avoid** `dynamic_cast` when possible. It requires RTTI and adds runtime complexity.

13. **Disable RTTI** via compiler flags:
    - GCC/Clang - compile with `-fno-rtti`
    - MSVC - compile with `/GR-`

14. **Use** `enum class` for exception-like and error-codes use-cases, instead of standard exceptions.
    - ❔ It reduces resource usage and increases predictability.
15. Handle errors and failures!
16. **Validate correctness of input** from outside (server, sensors, input devices, ...).
17. **Validate correctness of input** at the beginning of every function.

18. **Use meaningful names**.
19. **Avoid numbers**. Use meaningful constants and enums.
20. **Explain complex** or non-intuitive logic in comments.
21. Prefer **good naming over comments**.
22. **Write self-explanatory code**, avoid tricky solutions.
23. **Document** every function and class in a Doxygen-style comment, e.g., `/** ... */` before functions and classes, `///<` after constants/variable declarations in the same line. For example:
```c++
/**
 * @class Circle
 * @brief Represents a geometric circle and provides area calculation.
 */
class Circle {
public:
    /**
     * @brief Constructs a circle with the given radius.
     * @param r The radius of the circle.
     */
    Circle(double r) : radius(r) {}

    /**
     * @brief Calculates the area of the circle.
     * @return The computed area.
     */
    double area() const {
        return 3.14159 * radius * radius;
    }

private:
    double radius; ///< Radius of the circle
};
```

24. **Before every add or change** (aka push) into main branch, **validate it compiles without errors and doesn't add new warnings**.
25. When pushing commits, write concise yet explanatory comments to the commit.

If anything is unclear or missing, let us know: daphi.dnw@gmail.com.


## Coding Style Guide
This section focuses on code appearance rather than functionality, as consistent style improves readability and maintainability.

Since we couldn't find the upstream project's (Smart Citizen Kit's) style guide, we've adopted the [C++ Style Guide for Arduino Projects](https://www.makerguides.com/c-style-guide-for-arduino-projects/) as it closely matches the existing conventions.

**Note:**
- If existing code doesn't follow the style guide, update it to comply with it.
- If conflicts arise between this style guide and the coding guidelines above, **follow the coding guidelines**.


## Working on Issues
If you're picking up an issue to work on, please follow these steps:
1. **Comment to Claim** - Let others know you're working on it by commenting: "I'd like to work on this."
2. **Check for Details**  - Read the issue carefully. If anything is unclear, *ask questions before starting*.
3. **Follow Project Standards** - Stick to the coding guidelines, style guide, and contribution rules defined in the repo.
4. **Keep the Issue Updated** - Share progress or blockers in the comments so maintainers and others stay informed.
5. **Link Your Pull Request** - When ready, open a PR *and mention the issue* (issue number and title, e.g., `Issue Title #123`) to link them.
6. **Be Respectful & Collaborative** - Engage constructively with feedback and be open to suggestions.


## Getting more involved
If you want to get more invlolved, please contact us: daphi.dnw@gmail.com
