# Dev

## Linting and type checking

1. Install pyright and ruff vscode extensions. The former is for type checking, the latter is for formatting.

2. Add the follwing to your `.vscode/settings.json`
```json
{
  "[python]": {
    // Set Ruff as the default formatter for Python files
    "editor.defaultFormatter": "charliermarsh.ruff",

    // Automatically format code (indentation, spacing) on save
    "editor.formatOnSave": true,

    // Automatically fix linting errors (unused imports, sorting) on save
    "editor.codeActionsOnSave": {
      "source.fixAll": "explicit",
      "source.organizeImports": "explicit"
    }
  },

  // Enable Ruff linting (it reads from your pyproject.toml automatically)
  "ruff.enable": true,
  "ruff.lint.args": [],

  // Disable the built-in Pylint/Flake8 if you have them enabled,
  // since Ruff replaces them
  "python.linting.pylintEnabled": false,
  "python.linting.flake8Enabled": false,
  "python.linting.enabled": false, // Delegate linting to the Ruff extension

  // PYRIGHT CONFIGURATION
  // This tells VS Code to use the Pyright extension for type checking
  "python.analysis.typeCheckingMode": "basic", // Matches your pyproject.toml
  "python.analysis.extraPaths": [
    "scripts",
    "diff_testing"
  ]
}
```

- `uv run ruff format .` to format all files
- `uv run ruff check --fix .` to check formatting and fix issues

## Coverage

Pytket is built from source by the setup script. To override the prebuilt version, create a `local-override.toml` file with 
```
[tool.uv.sources]
pytket = { path = "external/tket/pytket", editable = true }
```

then set the environment variable `export UV_OVERRIDE="local-override.toml"`. All `uv` commands now run in the modified env.

