# /// script
# requires-python = ">=3.11"
# dependencies = [
#   "PyYAML==6.0.3",
# ]
# ///

import argparse
from random import randint, random, sample

import yaml


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Quest YAML test generator")
    parser.add_argument("-o", type=str, help="Output YAML file", required=True)
    parser.add_argument("-f", action="store_true", help="Do we use fast travel?")
    parser.add_argument("-b", action="store_true", help="Is graph bidirectional?")
    parser.add_argument("-w", action="store_true", help="Is graph weighted?")
    parser.add_argument("-v", type=int, help="Number of vertices", required=True)
    parser.add_argument("-vn", action="store_true", help="Do vertices have names?")
    parser.add_argument(
        "-d", type=float, help="Graph density in the range [0, 1]", required=True
    )
    parser.add_argument("-q", type=int, help="Number of quest lines", required=True)
    parser.add_argument(
        "-ql", type=int, help="Maximum quest line length", default=10
    )
    parser.add_argument("-qn", action="store_true", help="Do quest lines have names?")
    parser.add_argument("-s", type=int, help="Start vertex", default=-1)
    args = parser.parse_args()

    if args.v <= 0:
        parser.error("-v must be greater than zero")
    if not 0.0 <= args.d <= 1.0:
        parser.error("-d must be in the range [0, 1]")
    if args.q < 0:
        parser.error("-q must not be negative")
    if args.ql <= 0:
        parser.error("-ql must be greater than zero")
    if args.s != -1 and not 0 <= args.s < args.v:
        parser.error("-s must refer to an existing vertex")
    if args.v == 1 and args.ql > 1:
        args.ql = 1

    return args


def generate_graph(args: argparse.Namespace) -> dict:
    graph = {
        "fast_travel": args.f,
        "bidirectional": args.b,
        "weighted": args.w,
        "vertex_count": args.v,
    }

    if args.s != -1:
        graph["start"] = args.s

    if args.vn:
        graph["vertices"] = [
            {"id": vertex, "name": f"specific_vertex_name_{vertex}"}
            for vertex in range(args.v)
            if random() < 0.5
        ]

    all_edges = [
        (source, destination)
        for source in range(args.v)
        for destination in range(source + 1 if args.b else 0, args.v)
        if source != destination
    ]
    selected_edges = sample(all_edges, int(args.d * len(all_edges)))
    graph["edges"] = []
    for source, destination in selected_edges:
        edge = {"from": source, "to": destination}
        if args.w:
            edge["weight"] = round(100 * random(), 2)
        graph["edges"].append(edge)

    graph["quest_lines"] = []
    for quest_id in range(args.q):
        length = randint(1, args.ql)
        vertices = [randint(0, args.v - 1)]
        for _ in range(1, length):
            candidate = randint(0, args.v - 1)
            while candidate == vertices[-1]:
                candidate = randint(0, args.v - 1)
            vertices.append(candidate)

        quest_line = {"id": quest_id, "vertices": vertices}
        if args.qn and random() < 0.5:
            quest_line["name"] = f"quest_name_{quest_id}"
        graph["quest_lines"].append(quest_line)

    return graph


def main() -> None:
    args = parse_args()
    with open(args.o, "w", encoding="utf-8", newline="\n") as output:
        yaml.safe_dump(generate_graph(args), output, sort_keys=False, allow_unicode=True)


if __name__ == "__main__":
    main()
