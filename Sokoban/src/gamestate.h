#ifndef GAMESTATE_H
#define GAMESTATE_H

struct GLFWwindow;
class GraphicsManager;

class GameState {
public:
    GameState();
    virtual ~GameState();
    void create_child(std::unique_ptr<GameState> child);
    void set_graphics(GraphicsManager*);
    void set_csp(std::unique_ptr<GameState>*);
    virtual void main_loop() = 0;
    void check_for_escape_quit();
    bool check_for_queued_quit();
	void queue_quit();

	GraphicsManager* gfx_{};
	GLFWwindow* window_{};

private:
	void defer_to_parent();

	std::unique_ptr<GameState> parent_{};
	std::unique_ptr<GameState>* current_state_ptr_{};
	bool can_escape_quit_ = true;
	bool queued_quit_ = false;
};

#endif // GAMESTATE_H
