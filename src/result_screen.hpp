#include "globals.hpp"

class result_screen {
public:
	result_screen(results_struct results);
	void draw();
	void update();
private:
	results_struct results;
};