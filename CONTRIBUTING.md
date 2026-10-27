# Contributing to Wraith Launcher

Thanks for wanting to help! Wraith Launcher is a young project, and bug reports, ideas, documentation fixes and code are
all welcome. Everyone taking part is expected to follow the [Code of Conduct](CODE_OF_CONDUCT.md).

## Ways to help

- **Report a bug.** [Open an issue](https://github.com/Bacondude117/WraithLauncher/issues) with your Wraith version,
  your operating system, what you did, what you expected and what happened instead. Attach the launcher log or the
  game log if you can (the log viewer can copy them for you). Please search existing issues first.
- **Suggest a feature.** Open an issue describing the problem you want solved, not just the solution. Check
  [FEATURES.md](FEATURES.md) first to see whether it already exists. Ideas should fit the project's principles
  (lightweight, free, private, secure, fair to every mod source, and leaving the user in control; see the
  [README](README.md)). Features that add telemetry, ads, accounts or a favoured mod source won't be accepted.
- **Improve the docs.** Typos, unclear instructions and missing explanations are all worth a pull request.
- **Send code.** See below.

Problems that belong to Prism Launcher itself and also happen in Prism should go to the
[Prism Launcher project](https://github.com/PrismLauncher/PrismLauncher). Wraith is not affiliated with Prism, so
please don't report Wraith issues there.

## Sending a pull request

1. For anything bigger than a small fix, open an issue first so we can agree on the approach before you spend time on
   it.
2. Fork the repository and branch from `wraith-dev`, the main development branch.
3. Build and run the tests (see [Building from source](README.md#building-from-source)). New code must compile without
   warnings, because warnings are treated as errors.
4. Keep the pull request focused on one change, and describe what it does and why.
5. If your change adds a network request, list the endpoint in [PRIVACY.md](PRIVACY.md).
6. Make sure your commits are signed off (see below).

## Code style

All files are formatted with `clang-format` using the configuration in `.clang-format`. Run it on the files you
changed before committing.

Wraith follows these C++ conventions:

- Class and type names use `PascalCase`: `MyClass`.
- Private or protected data members use `camelCase` with an `m_` prefix: `m_myCounter`.
- Private or protected `static` data members use `camelCase` with an `s_` prefix: `s_instance`.
- Public data members use `camelCase` with no prefix: `dateOfBirth`.
- `static const` data members use `SCREAMING_SNAKE_CASE`: `MAX_VALUE`.
- Member functions use `camelCase` with no prefix: `incrementCounter`.
- Global functions and non-`const` global variables use `camelCase` with no prefix: `globalData`.
- `const` globals and macros use `SCREAMING_SNAKE_CASE`: `LIGHT_GRAY`.
- Enum constants use `PascalCase`: `HamAndPineapple`.
- Avoid inventing acronyms or abbreviations for multi-word names, like `tp` for `texturePack`.
- Avoid `[[nodiscard]]` unless ignoring the return value is likely to cause a bug, for example when a function
  allocates a resource the caller must clean up, or returns an error status alongside side effects. Plain getters
  don't need it.

Most of these rules are checked by `.clang-tidy`, so you can run `clang-tidy` to find violations. Here is what they
look like in practice:

```c++
#define AWESOMENESS 10

constexpr double PI = 3.14159;

enum class PizzaToppings { HamAndPineapple, OreoAndKetchup };

struct Person {
    QString name;
    QDateTime dateOfBirth;

    long daysOld() const { return dateOfBirth.daysTo(QDateTime::currentDateTime()); }
};

class ImportantClass {
   public:
    void incrementCounter()
    {
        if (m_counter + 1 > MAX_COUNTER_VALUE)
            throw std::runtime_error("Counter has reached limit!");

        ++m_counter;
    }

    int counter() const { return m_counter; }

   private:
    static constexpr int MAX_COUNTER_VALUE = 100;
    int m_counter;
};

ImportantClass importantClassInstance;
```

If you notice existing code that doesn't follow these conventions, leave it alone unless you're already refactoring
that whole class. Renames make changes harder to review and more likely to conflict.

## Signing your work

To make sure the code you contribute is compatible with the licenses in this codebase, every commit must be signed
off. Add `-s` to your `git commit` command, or append this line to the commit message yourself:

```text
Signed-off-by: Your Name <your.email@example.com>
```

By signing off, you agree to the Developer's Certificate of Origin 1.1:

```text
Developer's Certificate of Origin 1.1

By making a contribution to this project, I certify that:

(a) The contribution was created in whole or in part by me and I
    have the right to submit it under the open source license
    indicated in the file; or

(b) The contribution is based upon previous work that, to the best
    of my knowledge, is covered under an appropriate open source
    license and I have the right under that license to submit that
    work with modifications, whether created in whole or in part
    by me, under the same open source license (unless I am
    permitted to submit under a different license), as indicated
    in the file; or

(c) The contribution was provided directly to me by some other
    person who certified (a), (b) or (c) and I have not modified
    it.

(d) I understand and agree that this project and the contribution
    are public and that a record of the contribution (including all
    personal information I submit with it, including my sign-off) is
    maintained indefinitely and may be redistributed consistent with
    this project or the open source license(s) involved.
```

You can also [cryptographically sign your commits][gh-signing-commits] and enable
[vigilant mode][gh-vigilant-mode] on GitHub.

## Using AI tools

AI coding assistants are allowed, including on Wraith itself, but you are fully responsible for what you submit:

- Read, understand and test everything before you submit it. Don't submit code or issue text you couldn't explain.
- Mention in the pull request if an AI tool wrote a significant part of the change.
- Low-effort, untested or unreviewed AI output (including invented bug reports) will be closed.
- Make sure you have the right to submit the result under the project's license, as the sign-off above requires.

## Licensing

Wraith Launcher is licensed under the [GPL-3.0-only](LICENSE). By contributing, you agree that your contribution is
released under the same license.

[gh-signing-commits]: https://docs.github.com/en/authentication/managing-commit-signature-verification/signing-commits
[gh-vigilant-mode]: https://docs.github.com/en/authentication/managing-commit-signature-verification/displaying-verification-statuses-for-all-of-your-commits
