#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <map>
#include <set>
#include <cassert>
#include <algorithm>
#include <iomanip>
#include "Graph.hpp"
#include "Timer.hpp"
#include "utils.hpp"
#include "Alignment.hpp"
#include "HillClimbing.hpp"
#include "Measure.hpp"
#include "EdgeCorrectness.hpp"
#include "InducedConservedStructure.hpp"
#include "SymmetricSubstructureScore.hpp"
#include "LargestCommonConnectedSubgraph.hpp"
#include "Graphlet.hpp"
#include "GraphletLGraal.hpp"
#include "NodeDensity.hpp"
#include "EdgeDensity.hpp"
#include "ShortestPathConservation.hpp"
#include "GoSimilarity.hpp"
#include "GoAverage.hpp"
#include "GoCoverage.hpp"
#include "Sequence.hpp"
#include "NodeCorrectness.hpp"
#include "WeightedEdgeConservation.hpp"
#include "Importance.hpp"
#include "MeasureCombination.hpp"
#include "Method.hpp"
#include "WeightedAlignmentVoter.hpp"
#include "ArgumentParser.hpp"
#include "LGraalWrapper.hpp"
#include "HubAlignWrapper.hpp"
#include "SANA.hpp"
#include "NoneMethod.hpp"
#include "GreedyLCCS.hpp"
#include "GenericLocalMeasure.hpp"
#include "RandomAligner.hpp"
#include "Experiment.hpp"
#include "ParameterEstimation.hpp"
#include "ComplementaryProteins.hpp"
#include "AlphaEstimation.hpp"

using namespace std;

const string projectFolder = "/extra/wayne0/preserve/nmamano/networkalignment";

#include "defaultArguments.hpp"

char stringArgs[][80] = {
  "-g1", "-g2", "-startalignment", "-wecnodesim", "-wavenodesim", "-method",
  "-truealignment", "-experiment", "-o", "-paramestimation", "-alphaestimation", "-alphafile",
  "-eval", "-k", "-l", "-fg1", "-fg2"};
char doubleArgs[][80] = {
  "-ec", "-s3", "-graphlet", "-noded", "-edged", "-wec",
  "-importance", "-sequence",
  "-t",
  "-rewire", "-go", "-alpha", "-lgraaliter",
  "-tnew", "-iterperstep", "-numcand", "-tcand", "-tfin",
  "-qcount", "-graphletlgraal",
  ""};
char boolArgs[][80] = {"-dbg", "-goavg", "-submit", "-qsub", "-autoalpha",
"-restart", "-detailedreport", ""};
char vectorArgs[][80] = {"-nodedweights", "-edgedweights", "-goweights", ""};

string makeScript(const vector<string>& argvs) {
  int argc = argvs.size();
  string scriptName = "tmp/submit";
  addUniquePostfixToFilename(scriptName, ".sh");
  scriptName += ".sh";

  ofstream fout(scriptName.c_str());
  fout << "#!/bin/bash" << endl;
  fout << "cd " << projectFolder << endl;
  for (int i = 0; i < argc; i++) {
    if (not strEq(argvs[i], "-qsub")) {
      fout << argvs[i] << " ";
    }
  }
  fout << endl;
  return scriptName;
}

void submitToCluster(const vector<string>& argvs) {
  string scriptName = makeScript(argvs);
  exec("chmod +x " + scriptName);
  execPrintOutput("qsub " + scriptName);
}

void createFolders(ArgumentParser& args) {
  createFolder("matrices");
  createFolder("matrices/autogenerated");
  createFolder("tmp");
  createFolder("alignments");
  createFolder("go");
  createFolder("go/autogenerated");
}

/*
The program requires that there exist the network files in GW format
in networks/g1name/g1name.gw and networks/g1name/g2name.gw.

The -g1 and -g2 arguments allow you to specify g1name and g2name directly.
These arguments assume that the files already exist.

The -fg1 and -fg2 arguments allow you to specify external files containing
the graph definitions (in either GW or edge list format). If these
arguments are used, -g1 and -g2 are ignored. g1Name and g2Name are deduced
from the file names (by removing the path and the extension). Then,
the network definitions are parsed and the necessary network files are created.

*/
void initGraphs(Graph& G1, Graph& G2, ArgumentParser& args) {
  string fg1 = args.strings["-fg1"], fg2 = args.strings["-fg2"];
  createFolder("networks");
  string g1Name, g2Name;
  createFolder("alignments/"+g1Name+"_"+g2Name);
  if (fg1 != "") {
    g1Name = extractFileNameNoExtension(fg1);
  } else {
    g1Name = args.strings["-g1"];
  }
  if (fg2 != "") {
    g2Name = extractFileNameNoExtension(fg2);
  } else {
    g2Name = args.strings["-g2"];
  }

  string g1Folder, g2Folder;
  g1Folder = "networks/"+g1Name;
  g2Folder = "networks/"+g2Name;
  createFolder(g1Folder);
  createFolder(g2Folder);
  createFolder(g1Folder+"/autogenerated");
  createFolder(g2Folder+"/autogenerated");

  string g1GWFile, g2GWFile;
  g1GWFile = g1Folder+"/"+g1Name+".gw";
  if (fileExists(g1GWFile) and fg1 != "") {
    cerr << "Warning: argument of -fg1 (" << fg1 << 
      ") ignored because there already exists a network named " << g1Name << endl;
  }
  g2GWFile = g2Folder+"/"+g2Name+".gw";
  if (fileExists(g2GWFile) and fg2 != "") {
    cerr << "Warning: argument of -fg2 (" << fg2 << 
      ") ignored because there already exists a network named " << g2Name << endl;
  }

  if (not fileExists(g1GWFile)) {
    if (fg1 != "") {
      if (fileExists(fg1)) {
        if (fg1.size() > 3 and fg1.substr(fg1.size()-3) == ".gw") {
          exec("cp "+fg1+" "+g1GWFile);
        } else {
          Graph::edgeList2gw(fg1, g1GWFile);
        }
      } else {
        error("File not found: "+fg1);
      }
    } else {
      error("File not found: "+g1GWFile);
    }
  }
  if (not fileExists(g2GWFile)) {
    if (fg2 != "") {
      if (fileExists(fg2)) {
        if (fg2.size() > 3 and fg2.substr(fg2.size()-3) == ".gw") {
          exec("cp "+fg2+" "+g2GWFile);
        } else {
          Graph::edgeList2gw(fg2, g2GWFile);
        }
      } else {
        error("File not found: "+fg2);
      }
    } else {
      error("File not found: "+g2GWFile);
    }
  }

  cerr << "Initializing graphs... ";
  Timer T;
  T.start();
  G1 = Graph::loadGraph(g1Name);
  G2 = Graph::loadGraph(g2Name);

  double rewiredFraction = args.doubles["-rewire"];
  if (rewiredFraction > 0) {
    if (rewiredFraction > 1) error("Cannot rewire more than 100% of the edges");
    G2.rewireRandomEdges(rewiredFraction);
  }
    
  if (G1.getNumNodes() > G2.getNumNodes()) error("G2 has less nodes than G1");
  if (G1.getNumEdges() == 0 or G2.getNumEdges() == 0) error ("One of the networks has 0 edges");
  cerr << "done (" << T.elapsedString() << ")" << endl;  
}

void initMeasures(MeasureCombination& M, Graph& G1, Graph& G2, ArgumentParser& args) {
  cerr << "Initializing measures... " << endl;
  Timer T;
  T.start();
  Measure *m;

  //if not true, init only basic measures and any measure necessary to run SANA
  bool detRep = args.bools["-detailedreport"];

  m = new EdgeCorrectness(&G1, &G2);
  M.addMeasure(m, args.doubles["-ec"]);

  m = new InducedConservedStructure(&G1, &G2);
  M.addMeasure(m);    

  m = new SymmetricSubstructureScore(&G1, &G2);
  M.addMeasure(m, args.doubles["-s3"]);
  m = new LargestCommonConnectedSubgraph(&G1, &G2);
  M.addMeasure(m);

  double wecWeight = args.doubles["-wec"];

  double nodedWeight = args.doubles["-noded"];
  if (detRep or nodedWeight > 0 or (wecWeight > 0 and args.strings["-wecnodesim"] == "noded")) {
    m = new NodeDensity(&G1, &G2, args.vectors["-nodedweights"]);
    M.addMeasure(m, nodedWeight);
  }

  double edgedWeight = args.doubles["-edged"];
  if (detRep or edgedWeight > 0 or (wecWeight > 0 and args.strings["-wecnodesim"] == "edged")) {
    m = new EdgeDensity(&G1, &G2, args.vectors["-edgedweights"]);
    M.addMeasure(m, edgedWeight);    
  }

  if (GoSimilarity::fulfillsPrereqs(&G1, &G2)) {
    double goWeight = args.doubles["-go"];
    if (detRep or goWeight > 0 or (wecWeight > 0 and args.strings["-wecnodesim"] == "go")) {
      m = new GoSimilarity(&G1, &G2, args.vectors["-goweights"]);
      M.addMeasure(m, goWeight);
    }
    if (args.bools["-goavg"]) {
      m = new GoAverage(&G1, &G2);
      M.addMeasure(m);
    }
    if (detRep) {
      m = new GoCoverage(&G1, &G2);
      M.addMeasure(m);
    }
  }

  if (Importance::fulfillsPrereqs(&G1, &G2)) {
    double impWeight = args.doubles["-importance"];
    if (detRep or impWeight > 0 or (wecWeight > 0 and args.strings["-wecnodesim"] == "importance")) {
      m = new Importance(&G1, &G2);
      M.addMeasure(m, impWeight);
    }
  }

  string blastFile = "sequence/scores/"+G1.getName()+"_"+G2.getName()+"_blast.out";
  if (fileExists(blastFile)) {
    double seqWeight = args.doubles["-sequence"];
    if (detRep or seqWeight > 0 or (wecWeight > 0 and args.strings["-wecnodesim"] == "sequence")) {
      m = new Sequence(&G1, &G2);
      M.addMeasure(m, seqWeight);
    }
  }
 
  double graphletWeight = args.doubles["-graphlet"];
  if (detRep or graphletWeight > 0 or (wecWeight > 0 and args.strings["-wecnodesim"] == "graphlet")) {
    m = new Graphlet(&G1, &G2);
    M.addMeasure(m, graphletWeight);
  }
  double graphletLgraalWeight = args.doubles["-graphletlgraal"];
  if (detRep or graphletLgraalWeight > 0 or (wecWeight > 0 and args.strings["-wecnodesim"] == "graphletlgraal")) {
    m = new GraphletLGraal(&G1, &G2);
    M.addMeasure(m, graphletLgraalWeight);
  }

  if (detRep or wecWeight > 0) {
    LocalMeasure* wecNodeSim = (LocalMeasure*) M.getMeasure(args.strings["-wecnodesim"]);
    m = new WeightedEdgeConservation(&G1, &G2, wecNodeSim);
    M.addMeasure(m, wecWeight);
  }

  if (G1.getNumNodes() == G2.getNumNodes()) { //assume that we are aligning a network with itself
    if (strEq(args.strings["-truealignment"], "")) {
      m = new NodeCorrectness(Alignment::identity(G1.getNumNodes()));
    }
    else {
      m = new NodeCorrectness(Alignment::loadMapping(args.strings["-truealignment"]));
    }
    M.addMeasure(m);
  }

  //m = new ShortestPathConservation(&G1, &G2);
  //M.addMeasure(m);

  M.normalize();
  cerr << "done (" << T.elapsedString() << ")" << endl;
}

Method* initMethod(Graph& G1, Graph& G2, ArgumentParser& args, MeasureCombination& M) {
  string name = args.strings["-method"];
  string startAName = args.strings["-startalignment"];

  if (strEq(name, "greedylccs")) {
    return new GreedyLCCS(&G1, &G2, startAName);
  }
  if (strEq(name, "wave")) {
    LocalMeasure* waveNodeSim = (LocalMeasure*) M.getMeasure(args.strings["-wavenodesim"]);
    return new WeightedAlignmentVoter(&G1, &G2, waveNodeSim);
  }
  if (strEq(name, "lgraal")) {
    double seconds = args.doubles["-t"]*60;
    double alpha = args.doubles["-alpha"];
    if (args.bools["-autoalpha"]) {
      string alphaFile = args.strings["-alphafile"];
      if (not fileExists(alphaFile)) error("Couldn't find file "+alphaFile);
      alpha = AlphaEstimation::getAlpha(alphaFile, "LGRAAL", G1.getName(), G2.getName());
      if (alpha == -1) alpha = args.doubles["-alpha"];
    }
    return new LGraalWrapper(&G1, &G2, alpha, args.doubles["-lgraaliter"], seconds);
  }
  if (strEq(name, "hubalign")) {
    double alpha = args.doubles["-alpha"];
    if (args.bools["-autoalpha"]) {
      string alphaFile = args.strings["-alphafile"];
      if (not fileExists(alphaFile)) error("Couldn't find file "+alphaFile);
      alpha = AlphaEstimation::getAlpha(alphaFile, "HubAlign", G1.getName(), G2.getName());
      if (alpha == -1) alpha = args.doubles["-alpha"];
    }    
    //in hubalign alpha is the fraction of topology
    return new HubAlignWrapper(&G1, &G2, 1 - alpha);
  }
  if (strEq(name, "sana")) {
    cerr << "=== SANA -- optimize: ===" << endl;
    M.printWeights(cerr);
    cerr << endl;

    double k;
    if (args.strings["-k"] == "auto") k = 0;
    else k = stod(args.strings["-k"]);
    double l;
    if (args.strings["-l"] == "auto") l = 0;
    else l = stod(args.strings["-l"]);

    double minutes = args.doubles["-t"];

    if (args.bools["-autoalpha"]) {
      cout << "Outdated; needs to be refactored" << endl;
      // string methodName = "SANA";
      // string alphaFile = args.strings["-alphafile"];
      // if (not fileExists(alphaFile)) error("Couldn't find file "+alphaFile);
      // alpha = AlphaEstimation::getAlpha(alphaFile, methodName, G1.getName(), G2.getName());
      // if (alpha == -1) alpha = args.doubles["-alpha"];
    }

    Method* method = new SANA(&G1, &G2, k, l, minutes, &M);    
    if (args.bools["-restart"]) {
      double tnew = args.doubles["-tnew"];
      uint iterperstep = args.doubles["-iterperstep"];
      uint numcand = args.doubles["-numcand"];
      double tcand = args.doubles["-tcand"];
      double tfin = args.doubles["-tfin"];
      ((SANA*) method)->enableRestartScheme(tnew, iterperstep, numcand, tcand, tfin);
    }
    if (args.strings["-k"] == "auto") {
      ((SANA*) method)->setKAutomatically();
    }
    if (args.strings["-l"] == "auto") {
      ((SANA*) method)->setLAutomatically();
    }

    return method;
  }
  if (strEq(name, "hc")) {
    return new HillClimbing(&G1, &G2, &M, startAName);
  }
  if (strEq(name, "random")) {
    return new RandomAligner(&G1, &G2);
  }
  if (strEq(name, "none")) {
    return new NoneMethod(&G1, &G2, startAName);
  }
  throw runtime_error("Error: unknown method: " + name);
}

void makeReport(const Graph& G1, Graph& G2, const Alignment& A,
  const MeasureCombination& M, Method* method, ofstream& stream) {
  int numCCsToPrint = 3;

  stream << endl << currentDateTime() << endl << endl;

  stream << "G1: " << G1.getName() << endl;
  G1.printStats(numCCsToPrint, stream);
  stream << endl;

  stream << "G2: " << G2.getName() << endl;
  G2.printStats(numCCsToPrint, stream);
  stream << endl;

  stream << "Method: " << method->getName() << endl;
  method->describeParameters(stream);

  stream << endl << "execution time = " << method->getExecTime() << endl;

  stream << endl << "Scores:" << endl;
  M.printMeasures(A, stream);
  stream << endl;

  // string compareAFile = args.strings["-compare"];
  // if (not strEq(compareAFile, "")) {
  //   stream << "Alignment comparison:" << endl;
  //   vector<ushort> compareA = loadAlignment(compareAFile);
  //   stream << compareAFile << " = " << alignmentSimilarity(A, compareA) << endl;
  // }

  Graph CS = A.commonSubgraph(G1, G2);
  stream << "Common subgraph:" << endl;
  CS.printStats(numCCsToPrint, stream);
  stream << endl;
  
  int tableRows = min(5, CS.getConnectedComponents().size())+2;
  vector<vector<string> > table(tableRows, vector<string> (8));

  table[0][0] = "Graph"; table[0][1] = "n"; table[0][2] = "m"; table[0][3] = "alig-edges";
  table[0][4] = "indu-edges"; table[0][5] = "EC";
  table[0][6] = "ICS"; table[0][7] = "S3";

  table[1][0] = "G1"; table[1][1] = to_string(G1.getNumNodes()); table[1][2] = to_string(G1.getNumEdges());
  table[1][3] = to_string(A.numAlignedEdges(G1, G2)); table[1][4] = to_string(G2.numNodeInducedSubgraphEdges(A.getMapping()));
  table[1][5] = to_string(M.eval("ec",A));
  table[1][6] = to_string(M.eval("ics",A)); table[1][7] = to_string(M.eval("s3",A));

  for (int i = 0; i < tableRows-2; i++) {
    const vector<ushort>& nodes = CS.getConnectedComponents()[i];
    Graph H = CS.nodeInducedSubgraph(nodes);
    Alignment newA(nodes);
    newA.compose(A);
    table[i+2][0] = "CCS_"+to_string(i); table[i+2][1] = to_string(H.getNumNodes());
    table[i+2][2] = to_string(H.getNumEdges());
    table[i+2][3] = to_string(newA.numAlignedEdges(H, G2)); 
    table[i+2][4] = to_string(G2.numNodeInducedSubgraphEdges(newA.getMapping()));
    EdgeCorrectness ec(&H, &G2);
    table[i+2][5] = to_string(ec.eval(newA));
    InducedConservedStructure ics(&H, &G2);
    table[i+2][6] = to_string(ics.eval(newA));
    SymmetricSubstructureScore s3(&H, &G2);
    table[i+2][7] = to_string(s3.eval(newA));
  }

  stream << "Common connected subgraphs:" << endl;
  printTable(table, 2, stream);
  stream << endl;
}

void saveReport(const Graph& G1, Graph& G2, const Alignment& A,
  const MeasureCombination& M, Method* method, string reportFile) {
  string G1Name = G1.getName();
  string G2Name = G2.getName();
  if (strEq(reportFile, "")) {
    reportFile = "alignments/" + G1Name + "_" + G2Name + "/"
    + G1Name + "_" + G2Name + "_" + method->getName() + method->fileNameSuffix(A);
    addUniquePostfixToFilename(reportFile, ".txt");
    reportFile += ".txt";
  }

  ofstream outfile;
  outfile.open(reportFile.c_str());
  cerr << "Saving report as \"" << reportFile << "\"" << endl;
  A.write(outfile);
  makeReport(G1, G2, A, M, method, outfile);
  outfile.close();
}

void dbgMode(Graph& G1, Graph& G2, ArgumentParser& args) {
  // printLocalTopologicalSimilarities(G1, G2, 1);
  // exit(0);

  // LGraalObjectiveFunction m(&G1, &G2, 0);
  // GenericLocalMeasure m2(&G1, &G2, "arst", m.getSims());
  // m2.writeSimsWithNames("x");

  printProteinPairCountInNetworks(false);
  Alignment A = Alignment::loadEdgeList(&G1, &G2, "ignore/dijkstra_alignment.txt");
  printComplementaryProteinCounts(A, false);
  exit(0);

  // printProteinPairCountInNetworks(true);
  // string alig;
  // while (cin >> alig) {
  //   cout << alig << endl;
  //   Alignment A = Alignment::loadMapping(alig);
  //   printComplementaryProteinCounts(A, true);
  //   cout << endl;
  // }
  // exit(0);


  // Graph::GeoGeneDuplicationModel(5, 5, "geo5.gw");
  // Graph::GeoGeneDuplicationModel(10, 25, "geo10.gw");
  // Graph::GeoGeneDuplicationModel(20, 20*5, "geo20.gw");
  // Graph::GeoGeneDuplicationModel(40, 40*5, "geo40.gw");
  // Graph::GeoGeneDuplicationModel(80, 80*5, "geo80.gw");
  // Graph::GeoGeneDuplicationModel(160, 160*5, "geo160.gw");
  // Graph::GeoGeneDuplicationModel(320, 320*5, "geo320.gw");
  // Graph::GeoGeneDuplicationModel(640, 640*5, "geo640.gw");
  // Graph::GeoGeneDuplicationModel(1280, 1280*5, "geo1280.gw");
  // Graph::GeoGeneDuplicationModel(2560, 2560*5, "geo2560.gw");
  // Graph::GeoGeneDuplicationModel(5120, 5120*5, "geo5120.gw");
  // Graph::GeoGeneDuplicationModel(10240, 10240*5, "geo10240.gw");
  // Graph::GeoGeneDuplicationModel(20480, 20480*5, "geo20480.gw");

  // uint n = G1.getNumNodes();
  // while (n >= 5){
  //   n /= 2;
  //   cerr << n;
  //   G1 = G1.randomNodeInducedSubgraph(n);
  //   G1.saveInGWFormat("subgeo"+toString(n)+".gw");
  // }
  // exit(0);

  // vector<vector<string> > networks = fileToStringsByLines("experiments/allgeo.exp");
  // cout << "G1\tG2\tLogSearchSpace\tHC_iters\n";
  // for (uint i = 0; i < networks.size(); i++) {
  //   G1 = Graph::loadGraph(networks[i][0]);
  //   G2 = Graph::loadGraph(networks[i][1]);
  //   MeasureCombination M;
  //   initMeasures(M, G1, G2, args);
  //   Method* method = initMethod(G1, G2, args, M);
  //   double x = ((SANA*) method)->searchSpaceSizeLog();
  //   double y = ((SANA*) method)->hillClimbingIterations();
  //   cout << networks[i][0] << "\t" << networks[i][1] << "\t" << to_string(x) << "\t" << to_string(y) << endl; 
  // }
  MeasureCombination M;
  initMeasures(M, G1, G2, args);
  Method* method = initMethod(G1, G2, args, M);
  ((SANA*) method)->setTemperatureScheduleAutomatically();
}

int main(int argc, char* argv[]) {

  ArgumentParser args(stringArgs, doubleArgs, boolArgs, vectorArgs);
  args.parse(getArgumentList(argc, argv, defaultArguments, true));

  if (args.bools["-qsub"]) {
    for (int i = 0; i < args.doubles["-qcount"]; i++) {
      vector<string> argvs(argc);
      for (int i = 0; i < argc; i++) argvs[i] = argv[i];
      if (i > 0 and not strEq(args.strings["-o"], "")) {
        for (int j = 0; j < argc; j++) {
          if (strEq(argvs[j], "-o")) {
            argvs[j+1] += "_" + toString(i+1);
          }
        }
      }
      submitToCluster(argvs);
    }
    exit(0);
  }

  args.writeArguments();

  string exper = args.strings["-experiment"];
  if (not strEq(exper, "")) {
    string experFile = "experiments/"+exper+".exp";
    assert(fileExists(experFile));
    Experiment e(experFile);
    e.printData("experiments/"+exper+".txt");
    e.printDataCSV("experiments/"+exper+".csv");
    return 0;    
  }
  string paramEstimation = args.strings["-paramestimation"];
  if (not strEq(paramEstimation, "")) {
    string experFile = "experiments/"+paramEstimation+".exp";
    assert(fileExists(experFile));
    ParameterEstimation pe(experFile);
    if (args.bools["-submit"]) {
      pe.submitScriptsToCluster();
    }
    else {
      pe.collectData();
      pe.printData("experiments/"+paramEstimation+".out");
    }
    return 0;    
  }
  string alphaEstimation = args.strings["-alphaestimation"];
  if (not strEq(alphaEstimation, "")) {
    string experFile = "experiments/"+alphaEstimation;
    assert(fileExists(experFile));
    AlphaEstimation ae(experFile);
    ae.printData(experFile+".out");
    return 0;    
  }

  createFolders(args);
  Graph G1, G2;
  initGraphs(G1, G2, args);

  if (args.bools["-dbg"]) {
    dbgMode(G1, G2, args);
  }

  MeasureCombination M;
  initMeasures(M, G1, G2, args);

  Method* method;
  Alignment A = Alignment::empty();
  string aligFile = args.strings["-eval"];
  if (aligFile != "") {
    method = new NoneMethod(&G1, &G2, aligFile);
    A = Alignment::loadEdgeList(&G1, &G2, aligFile);
  }
  else {
    method = initMethod(G1, G2, args, M);
    A = method->runAndPrintTime();
  }

  A.printDefinitionErrors(G1,G2);
  assert (A.isCorrectlyDefined(G1, G2) and "Resulting alignment is not correctly defined");

  saveReport(G1, G2, A, M, method, args.strings["-o"]);
}
