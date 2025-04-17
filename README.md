# Gist
Imagine, you are playing in enormous RPG game with huge map
and big amount of quests. You are "100%" hunter,so you have
read all materials about game and know how pass it, but you still need an optimal route. This app can help you.

### Example of usage
Use ```quest_test_generator``` to make example 
```bash
  -h, --help  show this help message and exit
  -o O        Output file
  -f          Do we use fast travel?
  -b          Is graph bidirectional?
  -w          Is graph weighted?
  -v V        Number of vertexes
  -vn         Do vertexes have names?
  -d D        Density of graph (count of edges)
  -q Q        Number of quest lines
  -ql QL      Maximum length of quest line
  -qn         Do quest lines have names?
  -s S        Start vertex
```
```quest_test_generator.exe -q 20 -v 20 -d 0.3 -w -o example.txt```:
```txt
FastTravel:
	False
Bidirectional:
	False
Weighted:
	True
VertexCount:
	20
Edges:
	9 5 38.10
	7 13 31.62
	...
	15 3 38.82
	19 10 23.14
QuestLines:
	7 12 7 1 6 0 1
	2 19 11 6 11 17 13
	...
	11 8
```
That means:
- You can't use fast travel in your game
- Graph is directed (you can't go backward)
- Edges are weighted
- There are 20 vertexes and pack of edges with its
- There are different quest lines, e.g. ```7 12 7 1 6 0 1``` means that you have to 
visit 7th vertex, 12th vertex ... (exactly in such order) to complete this quest

Now let's try to find best

run ```cmake-build-release-mingw\bin\QuestOptimizerX.exe --file "path\to\example.txt" --num_threads 12 --max_queue_size 10000 --error_afford 1.1 --deep_of_search 200 --queue_narrowness 0.4 --log_interval_seconds 0.1 --disable_vertex_names --disable_quest_line_names```

### Options:
- ```num_threads``` - count of using threads
- ```max_queue_size``` - size of optimizer's queue (bounded by ~600000 for 16GB RAM, use bigger queue for better search)
- ```error_afford``` - heuristic for optimizer (use 1.00-1.05 for the fastest search, 1.06-1.2 for optimal search, bigger value can make result worse)
- ```deep_of_search``` - count of path that optimizer must find before choose the best one
- ```queue_narrownes``` - heuristic for optimizer (use 0.01-0.3 for the fastest search, 0.3-0.9 for balanced one, >= 1.0 is harmful)
- ```log_interval_seconds``` - show logging info, can be disabled by setting this option 0

### Output:
```
2795.11
0:3 10
4:3 15
5:4
7:0
1:15
2:1 16
8:
3:7
```

That means:
- Optimizer has found path with 2795.11 total length
- To pass it you must go to the vertex 0 and make progress for 3rd and 10th quests,
after that go to the 4th vertex and make progress for 3rd and 15th quests and so on...