#!/usr/bin/env python3
import argparse
import json
from pathlib import Path


def build_header(json_text: str, variable_name: str) -> str:
    parsed = json.loads(json_text)
    pretty_json = json.dumps(parsed, indent=2)
    return (
        '#pragma once\n\n'
        f'const char {variable_name}[] = R"json(\n'
        f'{pretty_json}\n'
        ')json";\n'
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Generate an Arduino header that embeds a coordinates JSON file.'
    )
    parser.add_argument('input_json', help='Path to the source JSON file.')
    parser.add_argument(
        '-o',
        '--output',
        help='Path to the output header. Defaults to <input_stem>_json.h next to the input file.',
    )
    parser.add_argument(
        '--variable-name',
        default='COORDINATES_JSON',
        help='Name of the generated C string variable. Defaults to COORDINATES_JSON.',
    )
    args = parser.parse_args()

    input_path = Path(args.input_json)
    output_path = Path(args.output) if args.output else input_path.with_name(f'{input_path.stem}_json.h')

    header_text = build_header(input_path.read_text(encoding='utf-8'), args.variable_name)
    output_path.write_text(header_text, encoding='utf-8', newline='\n')

    print(f'Wrote {output_path}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
