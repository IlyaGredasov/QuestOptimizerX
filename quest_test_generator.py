from os import linesep
from random import randint, random, sample
import argparse

parser = argparse.ArgumentParser(description="Quest test generator")

parser.add_argument("-o", type=str, help="Output file", required=True)
parser.add_argument("-f", action="store_true", help="Do we use fast travel?")
parser.add_argument("-b", action="store_true", help="Is graph bidirectional?")
parser.add_argument("-w", action="store_true", help="Is graph weighted?")
parser.add_argument("-v", type=int, help="Number of vertexes", required=True)
parser.add_argument("-vn", action="store_true", help="Do vertexes have names?")
parser.add_argument("-d", type=float, help="Density of graph (count of edges)", required=True)
parser.add_argument("-q", type=int, help="Number of quest lines", required=True)
parser.add_argument("-ql", type=int, help="Maximum length of quest line", required=False, default=10)
parser.add_argument("-qn", action="store_true", help="Do quest lines have names?")
parser.add_argument("-s", type=int, help="Start vertex", required=False, default=-1)

args = parser.parse_args()

with open(args.o, "w", newline="") as f:
    f.write(f"FastTravel:{linesep}")
    f.write(f"\t{args.f}{linesep}")

    f.write(f"Bidirectional:{linesep}")
    f.write(f"\t{args.b}{linesep}")

    f.write(f"Weighted:{linesep}")
    f.write(f"\t{args.w}{linesep}")

    f.write(f"VertexCount:{linesep}")
    f.write(f"\t{args.v}{linesep}")

    if args.s != -1:
        f.write(f"Start:{linesep}")
        f.write(f"\t{args.s}{linesep}")

    if args.vn:
        f.write(f"Vertexes:{linesep}")
        for i in range(args.v):
            if random() < 0.5:
                f.write(f"\t{i} specific_vertex_name_{i}{linesep}")

    all_edges = [(i, j) for i in range(args.v) for j in range(i + 1 if args.b else 0, args.v) if i != j]
    edges = sample(all_edges, int(args.d * len(all_edges)))

    f.write(f"Edges:{linesep}")
    for edge in edges:
        f.write(f'\t{edge[0]} {edge[1]} {f"{100 * random():.2f}" if args.w else ""}{linesep}')

    f.write(f"QuestLines:{linesep}")
    quest_lines = []
    for i in range(args.q):
        s = randint(1, args.ql)
        quest_line = s * [0]
        quest_line[0] = randint(0, args.v - 1)
        for j in range(1, s):
            quest_line[j] = randint(0, args.v - 1)
            while quest_line[j] == quest_line[j - 1]:
                quest_line[j] = randint(0, args.v - 1)
        quest_lines.append(quest_line)
    for i, quest_line in enumerate(quest_lines):
        f.write("\t" + " ".join(map(str, quest_line)) + (
            f" quest_name_{i}" if args.qn and random() < 0.5 else "") + linesep)
