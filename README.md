# NovaBanc

> A terminal-based C++ banking application that simulates the core operations of a retail bank.

NovaBanc was built as a focused semester project to demonstrate solid object-oriented design, persistent storage, input validation, password security, and unit testing — all in a clean, maintainable C++17 codebase. It is intentionally scoped: complete enough to be realistic, small enough that every line of it can be explained in a 30-minute technical interview.

---

## Features

**User Management**
- Register with full validation (name, email, phone, age, password)
- View and update profile (email, phone, password)
- SHA-256 + salt password hashing — plaintext never stored
- Account lockout after 3 consecutive failed login attempts

**Account Management**
- Open savings accounts (minimum Rs.500 initial deposit, 3.5% p.a. interest)
- Open current accounts (minimum Rs.1,000, Rs.5,000 overdraft facility)
- Close accounts (balance must be zero)
- Freeze / unfreeze accounts (admin)

**Financial Operations**
- Deposit to any active account
- Withdraw with minimum balance and daily limit enforcement
- Transfer between own accounts or to any account by ID
- Full transaction history with type filtering
- Balance inquiry

**Admin Portal**
- View all users and accounts
- Search users by name or email
- Lock / unlock users
- Delete users (only if no open accounts)
- Apply monthly interest to all savings accounts
- Export reports: users, transactions, account summary, per-account statement (CSV)
- Bank statistics dashboard

**Security & Audit**
- Passwords stored as SHA-256(password + salt)
- Automatic lockout after 3 failed login attempts
- Confirmation prompts before all destructive actions
- Every login, deposit, withdrawal, and transfer logged to `logs/novabanc.log`
- Admin uses separate hardcoded credentials (not a user record in the system)

---

## Prerequisites

- `g++ 6.3+` (or higher) with C++17 support
- GNU `make` (or `mingw32-make` on Windows)
- Linux, macOS, or Windows with MSYS2/MinGW/WSL

Third-party libraries are already bundled in `third_party/`:
- **nlohmann/json v3.11.3** — JSON serialization
- **Catch2 v2.13.10** — unit testing framework

---

## Build & Run

```powershell
# Build static release binary (using MSYS2/MinGW g++ in PowerShell)
g++ -std=c++17 -Wall -Wextra -Iinclude -Ithird_party -O2 -static (Get-ChildItem src\*.cpp) -o build\novabanc.exe

# Run the compiled executable
.\build\novabanc.exe
```

**Login credentials:**
- Register a new user from the main menu.
- Admin login: username `admin`, password `admin123`.

---

## Running Tests

```bash
make test
```

Runs all Catch2 test cases. Expected output: all tests pass.

To build with AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
make debug
./build/novabanc_debug
```

---

## Folder Structure

```
novabanc/
├── Makefile
├── README.md
├── .gitignore
├── include/          # All header files
├── src/              # All implementation files + main.cpp
├── tests/            # Catch2 test files
├── data/             # JSON persistence (users, accounts, transactions)
├── reports/          # Generated CSV reports (runtime)
├── logs/             # Audit log (runtime)
└── third_party/
    ├── nlohmann/     # json.hpp single-header
    └── catch2/       # Catch2 amalgamated
```

---

## Key Design Decisions

### Why object-oriented?

The core domain maps naturally to objects. A `User` has identity and state. An `Account` has a balance and a list of transactions. Making `Account` abstract allows `SavingsAccount` and `CurrentAccount` to share all the common logic — balance, transaction recording, status management — while implementing their own withdrawal rules through virtual methods. Adding a `FixedDepositAccount` later would mean writing one new subclass, not modifying existing code. This is the open/closed principle in practice.

### Why JSON, and why nlohmann/json?

I needed persistent storage that was human-readable (so I could inspect the data files during development), required no server or database setup, and used a well-documented library. nlohmann/json is a single header file, so there is no build configuration to manage. Every class that needs to persist implements `toJson()` and `fromJson()`, keeping serialization logic close to the data it describes.

### Why Catch2?

The `TEST_CASE` and `REQUIRE` macros read almost like plain English, which makes it easy to see at a glance what a test is asserting. The amalgamated release means no CMake integration is needed — it compiles with the same `g++` invocation as everything else. I targeted >80% line coverage by testing every exception path as well as the happy path.

### How authentication works

Passwords are never stored in plaintext. When a `User` is created, a 16-byte random salt is generated, and the password is stored as SHA-256(password + salt). Verification re-hashes the input with the stored salt and compares. The SHA-256 implementation is Brad Conte's public-domain algorithm (about 80 lines), included directly to avoid an OpenSSL dependency.

After 3 consecutive failed login attempts, the account is automatically locked. An administrator must unlock it. The admin account uses a hardcoded hash rather than a user record in the system, so it cannot be accidentally deleted or locked out.

### How storage works

`StorageManager` is the only class that reads or writes files. `Bank` calls it; nothing else does. This separation means that if the storage format ever needed to change (e.g., switch to SQLite), only `StorageManager` would change. On load, the `account_type` field in the JSON determines whether to construct a `SavingsAccount` or `CurrentAccount`. Every `nlohmann::json` parse is wrapped in a try/catch that throws `StorageException`, ensuring that corrupt data files produce a clean error rather than a crash.

---

## Known Limitations & Future Improvements

| Limitation | Reason / Future fix |
|---|---|
| No session tokens | A single "who is logged in" flag is correct for a single-user terminal app. A production system would use tokens with expiry. |
| No atomic file writes | Direct file writes are fine for a portfolio project. Production would write to a temp file and rename for atomicity. |
| No encryption at rest | Passwords are hashed. Future improvement: AES-256 encryption of the entire data directory. |
| Single-threaded | The architecture is correct for a terminal app. Multiple simultaneous users would require a server model, mutexes around shared state, and connection pooling. |
| No log rotation | One log file grows indefinitely. Production would use `logrotate` or an equivalent. |
| No FixedDeposit account type | `Savings` and `Current` fully demonstrate the polymorphism hierarchy. Adding `FixedDeposit` would mean writing a new subclass — no changes to existing code. |
| No GUI | Out of scope for a terminal C++ project. A web frontend could call a REST API layer wrapping `Bank`. |

---