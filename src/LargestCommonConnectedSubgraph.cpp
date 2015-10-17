#include "LargestCommonConnectedSubgraph.hpp"
#include <vector>
#include <cmath>
#include "Measure.hpp"
#include "Graph.hpp"
#include "utils.hpp"

LargestCommonConnectedSubgraph::LargestCommonConnectedSubgraph(Graph* G1, Graph* G2) : Measure(G1, G2, "lccs") {
}

LargestCommonConnectedSubgraph::~LargestCommonConnectedSubgraph() {
}

double LargestCommonConnectedSubgraph::eval(const Alignment& A) {
    Graph CS = A.commonSubgraph(*G1, *G2);
    vector<ushort> LCCSNodes = CS.getConnectedComponents()[0]; //largest CC
    uint n = LCCSNodes.size();
    double N = (double) n/G1->getNumNodes();
    if (not USE_MAGNA_DEFINITION) return N;

	Graph G1InducedSubgraph = G1->nodeInducedSubgraph(LCCSNodes);
	Graph G2InducedSubgraph = G2->nodeInducedSubgraph(LCCSNodes);
	double E1 = (double) G1InducedSubgraph.getNumEdges()/G1->getNumEdges();
	double E2 = (double) G2InducedSubgraph.getNumEdges()/G2->getNumEdges();
	double E = min(E1,E2);
	return sqrt(E*N);
}

double LargestCommonConnectedSubgraph::nodeProportion(const Alignment& A) {
    Graph CS = A.commonSubgraph(*G1, *G2);
    vector<ushort> LCCSNodes = CS.getConnectedComponents()[0]; //largest CC
    uint n = LCCSNodes.size();
    return (double) n/G1->getNumNodes();
}