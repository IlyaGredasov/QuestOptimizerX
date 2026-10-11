# QuestOptimizerX

QuestOptimizerX finds an efficient route through ordered quest objectives on a
weighted or unweighted graph.

## Build

The C++ application requires CLI11 and yaml-cpp. On Ubuntu/WSL they can be
installed with:

```bash
./install_cpp_requirements.sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake --build build -j
```

## Generate an input file

The generator is a self-contained `uv` script; `uv` installs its pinned PyYAML
dependency automatically:

```bash
uv run quest_test_generator.py -q 20 -v 20 -d 0.3 -w -o example.yaml
```

Useful generator options:

```text
-o FILE   output YAML file
-f        enable fast travel
-b        make the graph bidirectional
-w        generate weighted edges
-v N      number of vertices
-vn       add names to a random subset of vertices
-d D      edge density in the range [0, 1]
-q N      number of quest lines
-ql N     maximum quest line length
-qn       add names to a random subset of quest lines
-s N      fixed start vertex
```

The generated YAML has this shape:

```yaml
fast_travel: false
bidirectional: false
weighted: true
vertex_count: 4
start: 0
vertices:
  - id: 0
    name: village
  - id: 3
    name: castle
edges:
  - from: 0
    to: 1
    weight: 12.5
  - from: 1
    to: 3
    weight: 8.25
quest_lines:
  - id: 0
    name: main_quest
    vertices: [0, 1, 3]
  - id: 1
    vertices: [2, 3]
```

`start`, `vertices`, quest `id`, and quest `name` are optional. Edge `weight`
is required exactly when `weighted` is `true`. Unnamed vertices and quests get
the names `vertex_N` and `quest_N`.

## Run

Only `--file` is required; all search options have defaults:

```bash
./build/quest_optimizer_x \
  --file example.yaml \
  --num-threads 12 \
  --max-queue-size 10000 \
  --error-afford 1.1 \
  --depth-of-search 200 \
  --log-interval-seconds 0.1 \
  --enable-vertex-names \
  --enable-quest-line-names
```

The underscore spellings used by older commands, such as `--num_threads`, are
kept as aliases. Run `./build/quest_optimizer_x --help` for the complete CLI11
help output.

- `num-threads` controls the number of optimizer threads.
- `max-queue-size` bounds the search queue.
- `error-afford` controls heuristic pruning.
- `depth-of-search` is the number of complete paths to find before selecting
  the best one.
- `log-interval-seconds` controls periodic logging; zero disables it.

## Annotator

The image annotator is also a self-contained `uv` script and writes
`<image>_graph.yaml` next to the labeled image:

```bash
uv run quest_annotator.py path/to/map.png
```
