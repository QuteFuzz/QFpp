#!/usr/bin/env python3
import argparse
import sys
from pathlib import Path

import yaml
from jinja2 import Environment, FileSystemLoader, TemplateError, TemplateNotFound

PROJECT_ROOT = Path.cwd()
TEMPLATE_DIR = PROJECT_ROOT / "templates"
CONFIG_DIR = PROJECT_ROOT / "config"
OUTPUT_DIR = PROJECT_ROOT
GRAMMARS_DIR = PROJECT_ROOT / "grammar_definitions"


def parse_arguments():
    """
    Sets up and parses command line arguments.
    """
    parser = argparse.ArgumentParser(
        description="Generate C++ lexer headers from YAML configuration using Jinja2."
    )

    parser.add_argument(
        "-y",
        "--yaml",
        dest="yaml_file",
        default="tokens.yaml",
        help="Path to the input YAML configuration file (default: tokens.yaml)",
    )

    parser.add_argument(
        "-t",
        "--template",
        dest="template_file",
        default="lex.h.j2",
        help="Path to the input Jinja2 template file (default: lex.h.j2)",
    )

    parser.add_argument(
        "-o",
        "--output",
        dest="output_file",
        default="lex.h",
        help="Path to the output C++ header file (default: lex.h)",
    )

    return parser.parse_args()


def load_config(yaml_file: str):
    """
    Loads and parses the YAML configuration file.
    """
    yaml_path = CONFIG_DIR / yaml_file

    if not yaml_path.exists():
        print(f"Error: YAML file not found at '{yaml_path}'", file=sys.stderr)
        sys.exit(1)

    try:
        with open(yaml_path, "r") as f:
            data = yaml.safe_load(f)
            return data
    except yaml.YAMLError as exc:
        print(f"Error parsing YAML file: {exc}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected error loading YAML: {e}", file=sys.stderr)
        sys.exit(1)


def render_template(data, template_file: str, output_file: str):
    """
    Sets up the Jinja2 environment, renders the template with the data,
    and writes the result to the output file.
    """
    template_path = TEMPLATE_DIR / template_file
    output_path = OUTPUT_DIR / output_file

    if not template_path.exists():
        print(f"Error: Template file not found at '{template_path}'", file=sys.stderr)
        sys.exit(1)

    try:
        env = Environment(
            loader=FileSystemLoader(TEMPLATE_DIR), trim_blocks=False, lstrip_blocks=True
        )
        template = env.get_template(template_file)

        # Render the template
        output_content = template.render(
            enum_sections=data.get("enum_sections", []), matchers=data.get("matchers", [])
        )

    except TemplateNotFound:
        print(
            f"Error: Jinja2 could not find template '{template_file}' in '{TEMPLATE_DIR}'",
            file=sys.stderr,
        )
        sys.exit(1)
    except TemplateError as e:
        print(f"Error during template rendering: {e}", file=sys.stderr)
        sys.exit(1)

    # Write output
    try:
        with open(output_path, "w") as f:
            f.write(output_content)
        print(
            f"Successfully generated '{output_path}' using config ' \
                {data.get('yaml_source', 'YAML')}'"
        )
    except IOError as e:
        print(f"Error writing to output file '{output_path}': {e}", file=sys.stderr)
        sys.exit(1)


def add_grammar_file_tokens(config_data):
    """
    Go through grammars dir and add tokens for each of the grammars.
    These will be used later for scoping
    """
    enum_sections = config_data["enum_sections"]
    tokens = []

    for grammar_file in GRAMMARS_DIR.iterdir():
        tokens.append(grammar_file.stem.upper())

    for section in enum_sections:
        if section["start_marker"] == "GRAMMAR_SYNTAX_TOP":
            section["syntax_tokens"] += tokens


def main():
    args = parse_arguments()

    print("--- Lexer Generator ---")
    print(f"Input Config:   {args.yaml_file}")
    print(f"Input Template: {args.template_file}")
    print(f"Output File:    {args.output_file}")

    config_data = load_config(args.yaml_file)

    config_data["yaml_source"] = args.yaml_file
    add_grammar_file_tokens(config_data=config_data)

    render_template(config_data, args.template_file, args.output_file)


if __name__ == "__main__":
    main()
