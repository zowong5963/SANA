#ifndef GOSIMILARITY_HPP
#define	GOSIMILARITY_HPP
#include "LocalMeasure.hpp"
#include "Measure.hpp"
#include <unordered_map>

class GoSimilarity: public LocalMeasure {
public:
    GoSimilarity(Graph* G1, Graph* G2, const vector<double>& countWeights);
    virtual ~GoSimilarity();
    //double eval(const Alignment& A);

	static string getGoSimpleFileName(const Graph& G);
	static void ensureGoFileSimpleFormatExists(const Graph& G);

	static vector<vector<uint> > loadGOTerms(const Graph& G);

	//returns the number of times that each go term appears in G
	static unordered_map<uint,uint> getGoCounts(const Graph& G);

	static bool fulfillsPrereqs(Graph* G1, Graph* G2);

private:
	vector<double> countWeights;
	void initSimMatrix();

	static void assertNoRepeatedEntries(const vector<vector<uint> >& goTerms);
	static void simpleToInternalFormat(const Graph& G, string GOFileSimpleFormat, string GOFileInternalFormat);
	static void ensureGOFileInternalFormatExists(const Graph& G);

	static ushort numberAnnotatedProteins(const Graph& G);

	static void generateGOFileSimpleFormat(string GOFile, string GOFileSimpleFormat);
	static void generateGene2GoSimpleFormat();

	static bool isBioGRIDNetwork(const Graph& G);
	static bool hasGOData(const Graph& G);
};

#endif

