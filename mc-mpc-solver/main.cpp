#include <lemon/list_graph.h>
#include <lemon/cost_scaling.h>
#include <string.h>
#include <lemon/lgf_reader.h>
#include <lemon/lgf_writer.h>
#include "../util/utils.h"

using namespace lemon;
using namespace std;

string solve_minflow(string filename);

int main(int argc, char* argv[])
{
  if (argc != 2){
    cerr << "Usage: " << argv[0] << " GRAPH_FILENAME" << endl;
    return 1;
  }

  if(!file_exists(argv[1])){
    cerr << "ERROR: input file not found\n";
    return 1;
  }

  string result = solve_minflow(argv[1]);
  cout << result;

  return 0;
}

void transform_to_minflow_graph(ListDigraph& g, ListDigraph::ArcMap<int>& demands)
{
	//splits each node into connected nodes, where the first has all the incoming and the second all the outgoing edges

	for (ListDigraph::NodeIt n(g); n != INVALID; ++n)
	{
		ListDigraph::Node new_node = g.split(n, false);
		ListDigraph::Arc new_arc = g.addArc(n, new_node);
		demands[new_arc] = 1;
	}
}

string solve_minflow(string filename)
{

  ListDigraph g;
  ListDigraph::NodeMap<int> node_labels(g);
  ListDigraph::ArcMap<int> arc_labels(g);
  ListDigraph::ArcMap<int> arc_weights(g);
  digraphReader(g, filename)
    .nodeMap("label", node_labels)
    .arcMap("label", arc_labels)
    .arcMap("weight", arc_weights)
    .run();
  
  ListDigraph::ArcMap<bool> original_arc(g, false);
  for(ListDigraph::ArcIt ai(g); ai != INVALID; ++ai){
    original_arc[ai] = true;
  }

	// transform the graph so that every node is split in two, and demand for the edge between the duplicates is 1

	ListDigraph::ArcMap<int> demands(g);
	transform_to_minflow_graph(g, demands);

	//then add source and tap and connect source to all nodes with no incoming edges and all nodes with no outgoing edges to tap

	ListDigraph::Node s = g.addNode();
	ListDigraph::Node t = g.addNode();

	for (ListDigraph::NodeIt n(g); n != INVALID; ++n)
	{
		if (countInArcs(g, n) == 0)
			g.addArc(s, n);

		if (countOutArcs(g, n) == 0)
			g.addArc(n, t);
	}

	//initalize cost scaling algorithm and give it the demands and the weights of the edges

	CostScaling<ListDigraph> costScaling(g);
	costScaling.lowerMap(demands);
	costScaling.costMap(arc_weights);

	//pushing 100 units of flow is just arbitrary number, it has to be equal or bigger than the minimum flow
	costScaling.stSupply(s, t, 100);

	costScaling.run();

  ListDigraph::ArcMap<int> minflow(g);
  costScaling.flowMap(minflow);

  string result = "";
  for(ListDigraph::ArcIt ai(g); ai != INVALID; ++ai){
    //we have added many extra arcs which are not needed in the output
    if(original_arc[ai]){
      result += (to_string(arc_labels[ai]) + " " + to_string(minflow[ai]) + "\n");
    }
  }

  return result;
}