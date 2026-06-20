# ⊢ Logic Engine

A lightweight, high-performance propositional logic proof checker. It features a modern web-based frontend and a robust C++ backend that utilizes forward-chaining saturation to derive conclusions from provided premises using 19 standard rules of inference and replacement.

---

## 🚀 Features

- **19 Rules of Inference & Replacement:** Supports standard propositional rules (e.g., Modus Ponens, De Morgan's, Constructive Dilemma).
- **Forward-Chaining Saturation:** Automatically derives valid steps to determine if a conclusion follows from the given premises.
- **Modern Web UI:** A clean, responsive interface for inputting logic arguments, complete with syntax highlighting and a step-by-step proof breakdown.
- **High Performance:** C++ backend powered by the [Crow](https://github.com/CrowCpp/Crow) web framework.

---

## 🛠️ Tech Stack

| Layer | Technology |
|---|---|
| Frontend | HTML5, Vanilla JavaScript, CSS (Flexbox/Grid) |
| Backend | C++17, Crow (HTTP server) |
| Build System | CMake |

---

## 📋 Prerequisites

Ensure you have the following installed on your system:

- C++ Compiler (supporting C++17)
- CMake (version 3.16 or higher)
- Git

---

## 💻 Setup & Installation

### 1. Backend

Navigate to the `backend` directory, build the project, and start the server:

```bash
cd backend
cmake -B build
cmake --build build
./build/logic_server
```

The server will start at `http://localhost:8080`.

### 2. Frontend

Open the `frontend/index.html` file directly in any modern web browser.

---

## ✍️ Usage

1. **Enter Premises:** Input your premises in the textarea, one per line.
2. **Define Conclusion:** Input the target formula.
3. **Prove:** Click **▶ Prove Argument**.
4. **Analyze:** If the argument is valid, the UI will display a step-by-step derivation table showing how the conclusion was reached.

### Supported Syntax

| Element | Symbol(s) |
|---|---|
| Atoms | `p`, `q`, `rain`, `cloudy` |
| Negation | `~p` or `!p` |
| Conjunction | `p & q` or `p /\ q` |
| Disjunction | `p \| q` or `p \/ q` |
| Implication | `p -> q` |
| Biconditional | `p <-> q` |
