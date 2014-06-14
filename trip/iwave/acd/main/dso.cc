#include "acd_defn.hh"
#include "grid.h"
#include "istate.hh"
#include "iwop.hh"
#include "gridpp.hh"
#include "gridops.hh"
#include "functions.hh"
#include "cgnealg.hh"
#include "adjtest.hh"

//#define GTEST_VERBOSE
IOKEY IWaveInfo::iwave_iokeys[]
= {
  {"csq",    0, true,  true },
  {"data",   1, false, true },
  {"source", 1, true,  false},
  {"",       0, false, false}
};

using RVL::parse;
using RVL::RVLException;
using RVL::Vector;
using RVL::Components;
using RVL::Operator;
using RVL::OperatorEvaluation;
using RVL::LinearOp;
using RVL::LinearOpFO;
using RVL::OpComp;
using RVL::SymmetricBilinearOp;
using RVL::AssignFilename;
using RVL::AssignParams;
using RVL::RVLRandomize;
using RVL::AdjointTest;
using TSOpt::IWaveEnvironment;

#ifdef IWAVE_USE_MPI
using TSOpt::MPIGridSpace;
#else
using TSOpt::GridSpace;
#endif
using TSOpt::GridDerivOp;


int xargc;
char **xargv;

int main(int argc, char ** argv) {

  try {

#ifdef IWAVE_USE_MPI
    int ts=0;
    MPI_Init_thread(&argc,&argv,MPI_THREAD_FUNNELED,&ts);    
#endif
      
    PARARRAY * pars = NULL;
    FILE * stream = NULL;
    IWaveEnvironment(argc, argv, 0, &pars, &stream);
#ifdef IWAVE_USE_MPI
      if (retrieveGroupID() == MPI_UNDEFINED) {
          fprintf(stream,"NOTE: finalize MPI, cleanup, exit\n");
          fflush(stream);
      }
      else {
#endif
      
      
      /* generate physical model space */
#ifdef IWAVE_USE_MPI
      MPIGridSpace csqsp(valparse<std::string>(*pars,"csqext"),"notype",true);
#else
      GridSpace csqsp(valparse<std::string>(*pars,"csqext"),"notype",true);
#endif
          
      // make it a product, so it's compatible with domain of op
      StdProductSpace<ireal> dom(csqsp);
          int dsdir = INT_MAX;
          if (retrieveGlobalRank()==0) dsdir=csqsp.getGrid().dim;
          cerr << "\n after get grid dim \n";
          cerr << "\n" << retrieveGlobalRank() << "  dsdir = " << dsdir << endl;
#ifdef IWAVE_USE_MPI
          if (MPI_Bcast(&dsdir,1,MPI_INT,0,retrieveGlobalComm())) {
              RVLException e;
              e<<"Error: acddscheb_grad, rank="<<retrieveGlobalRank()<<"\n";
              e<<"  failed to bcast dsdir\n";
              throw e;
          }
#endif
      GridDerivOp dsop(dom,dsdir,valparse<float>(*pars,"DSWeight",1.0f));
          
          Vector<ireal> min(dom);
          Vector<ireal> mout(dom);
          
          AssignFilename minfn(valparse<std::string>(*pars,"csqext"));
          Components<ireal> cmin(min);
          cmin[0].eval(minfn);
          
          AssignFilename moutfn(valparse<std::string>(*pars,"dcsqextout"));
          Components<ireal> cmout(mout);
          cmout[0].eval(moutfn);
          
          RVLRandomize<float> rnd(getpid(),-1.0,1.0);
          
          OperatorEvaluation<ireal> opeval(dsop,min);
          AdjointTest<float>(opeval.getDeriv(),rnd,cerr);
          
          opeval.getDeriv().applyOp(min,mout);
      
#ifdef IWAVE_USE_MPI
          }
    MPI_Finalize();
#endif
  }
  catch (RVLException & e) {
    e.write(cerr);
#ifdef IWAVE_USE_MPI
    MPI_Abort(MPI_COMM_WORLD,0);
#endif
    exit(1);
  }
  
}