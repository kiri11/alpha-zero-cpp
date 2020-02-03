#ifndef NNWRAPPER_H     
#define  NNWRAPPER_H

#include <iostream>
#include <Eigen/Dense>
#include <Eigen/Core>
#include <torch/script.h>
#include "NNUtils.h"
#include <GameState.h>
//#include "NNObserver.h"

#include <experimental/filesystem>
#include <mutex>  
#include <shared_mutex>


using namespace Eigen;
namespace fs = std::experimental::filesystem;

//class NNObserver;   //forward declaration

class NNWrapper{
	private:
		torch::jit::script::Module module;
		std::unordered_map<std::string, NN::Output> netCache;
		
		fs::file_time_type modelLastUpdate;
		mutable std::shared_mutex modelMutex;
		//std::unique_ptr<NNObserver> observer; 
		
		bool inCache(std::string board);
		NN::Output getCachedEl(std::string board);
		void inserInCache(std::string board, NN::Output o);
		void load(std::string filename);

	public:
		NNWrapper(std::string filename);
		NN::Output maybeEvaluate(std::shared_ptr<GameState> leaf);
		std::vector<NN::Output> maybeEvaluate(std::vector<std::shared_ptr<GameState>> leafs);
		std::vector<NN::Output> predict(NN::Input input);

		//std::shared_mutex* getModelMutex();
		void shouldLoad(std::string filename);

		
};

#endif 