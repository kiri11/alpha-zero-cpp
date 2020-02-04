#include <NNWrapper.h>
#include <MCTS.h>
#include <Player.h>
#include <Game.h>
#include <random>
#include <json.hpp>
#include <experimental/filesystem>
#include <ctime>
#include <chrono>

using json = nlohmann::json;
namespace fs = std::experimental::filesystem;


namespace Selfplay{
	struct Config {
		int tempthreshold = 8;
		float afterThresholdTemp = 1.5;
		bool print = false;

		MCTS::Config mcts = { 
    		2, //cpuct 
    		1, //dirichlet_alpha
    		25, // n_simulations
    		0.1, //temp
    	};
	};
}


void save_game(std::shared_ptr<Game> game, 
	std::vector<std::vector<float>> probabilities, 
	std::vector<std::vector<std::vector<float>>> history
	){
	std::string directory = "./temp/games/";
	std::time_t t = std::time(0); 

	if (!fs::exists(directory)){
		fs::create_directories(directory);
	}
    
    int tid = omp_get_thread_num();
	std::ofstream o(directory + "game_" +  std::to_string(tid) +  "_" + std::to_string(t));
	json jgame;
	jgame["probabilities"] = probabilities;
	jgame["winner"] = game->getCanonicalWinner();
	jgame["history"] = history;

	o << jgame.dump() << std::endl;
}

void play_game(
	std::shared_ptr<Game> n_game, 
	NNWrapper& model, 
	Selfplay::Config cfg
	){

	std::shared_ptr<Game> game = n_game->copy();
	std::vector<std::vector<float>> probabilities;
	std::vector<std::vector<std::vector<float>>> history;

	std::random_device rd;
    std::mt19937 gen(rd());

    std::shared_ptr<GameState> gs =  std::make_shared<GameState>(game);

	int action;
	ArrayXf p;
	int game_length = 0;

	while (not game->ended()){
		//save board
    	Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> board(game->getBoard());

    	std::vector<std::vector<float>> b_v;
    	for (int i=0; i<board.rows(); ++i){
    		b_v.push_back(std::vector<float>(board.row(i).data(), board.row(i).data() + board.row(i).size()));
    	}
    	history.push_back(b_v);

    	//simulate
		p = MCTS::parallel_simulate(gs, model, cfg.mcts);

		//save probability
    	std::vector<float> p_v(p.data(), p.data() + p.size());
    	probabilities.push_back(p_v);

    	//playls
    	std::discrete_distribution<> dist(p.data(),p.data() +  p.size());
    	action = dist(gen);
    	game->play(action);
    	game_length++;
    	
    	gs = gs->getChild(action);

    	if (game_length > cfg.tempthreshold){
    		cfg.mcts.temp = cfg.afterThresholdTemp;
    	}

    	if (cfg.print){
			game->printBoard();
    	}
	}

	save_game(game, probabilities, history);
}

int main(int argc, char** argv){
	std::shared_ptr<Game> g = Game::create(argv[1]);
	int n_games = std::stoi(argv[3]);
	Selfplay::Config cfg; 


	std::cout << "n games:" << n_games<< std::endl;
	NNWrapper model = NNWrapper(argv[2]);
	#pragma omp parallel
	{	
	int i = 0;
	while (true){
		play_game(g, model, cfg);

		auto now = std::chrono::system_clock::now();
		std::time_t now_time = std::chrono::system_clock::to_time_t(now);
		std::cout<< std::ctime(&now_time) <<" Game Generated" << std::endl;

		#pragma omp critical
		{
			model.shouldLoad(argv[2]);
		}		

		i++;
		if (n_games > 0 && i == n_games){
			break;
		}
	}
	}
	return 0;
}
