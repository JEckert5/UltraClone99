#include "../headers/includes.hpp"

#include "../headers/Animation.hpp"
#include "../headers/Entity.hpp"
#include "../headers/Player.hpp"
#include "../headers/Bullet.hpp"
#include "../headers/Dio.hpp"

float DeltaTime;
std::mt19937 m;

float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

const int Width = 1000;
const int Height = 1000;

using namespace sf;

// Very bad stupid "Video" player class
class Video final : public Entity {
public:
	Video(){
        name = "Video";

        playing = false;
        accumulator = 0.0f;
        frame = 0;
        fps = 0;
    }

	/**
	 * \brief Load all necessary images. Images NEED to be in format 'output-XXXX.png'.
	 * \param frameFolder Path to folder containing images/frames
     * \param audioPath path to audio file
	 */
	void install(const std::string& frameFolder, const int numFrames, const float fps, const std::string& audioPath) {
        this->fps = 1 / fps;
        audio.openFromFile(audioPath);

        for (int i = 1; i <= numFrames; i++) {
            std::string file;

            if (i < 10) {
                file = "/output-000";
            } else if (i < 100) {
                file = "/output-00";
            } else if (i < 1000) {
                file = "/output-0";
            } else {
                file = "/output-";
            }

            Texture t;
            t.loadFromFile(frameFolder + file + std::to_string(i) + ".png");

            mFrames.emplace_back(t);
        }
    }

	void tick() override {
        if (!playing)
            return;

        accumulator += DeltaTime;

        if (accumulator >= fps) {
            accumulator = 0;
            frame += 1;
            if (!(frame >= mFrames.size())) {
                anim.sprite.setTexture(mFrames[frame]);
            } else {
                stop();
            }
        }
    }

	void start() {
        audio.play();
        playing = true;
        frame = 0;
        anim.sprite.setTexture(mFrames[frame]);
    }

	void stop() {
        audio.stop();
        playing = false;
        frame = 0;
        accumulator = 0;
    }

    bool isPlaying() const {
        return playing;
    }

    void draw(RenderWindow& window) override {
        window.draw(anim.sprite);
    }
private:
    bool playing;
    std::vector<Texture> mFrames;
    Music audio;
    int frame;
    float fps, accumulator;
};

int main() {
    std::random_device r;
    m.seed(r());
    bool Paused = false;

    Music music;
    music.openFromFile("audio/jojo.wav");

    Text dioHealth, playerHealth;
    Font font;
    dioHealth.setPosition(Width * 0.7f, Height * 0.75f);
    playerHealth.setPosition(Width * 0.2f, Height * 0.75f);

    if (!font.loadFromFile("images/MGS2.ttf")) {
        std::cout << "Could not load font" << std::endl;
        return -1;
    }

    dioHealth.setFont(font);
    playerHealth.setFont(font);
    dioHealth.setCharacterSize(50);
    playerHealth.setCharacterSize(50);
    
    RenderWindow window(VideoMode(Width, Height), "Touhous Bizzare Adventure");

    std::list<Entity*> entities;

    Texture pt, bt, bt2, et, bgr;
    pt.loadFromFile("images/player.png");
    bt.loadFromFile("images/bullet.png");
    bt2.loadFromFile("images/bullet-2.png");
    et.loadFromFile("images/enemy.png");
    bgr.loadFromFile("images/optimum prime.png");

    Animation ba, ba2, pa, ea, bgra;
    bgra = Animation(bgr, 0, 0, 400, 399, 1, 0.1f);
    pa = Animation(pt, 0, 0, 336, 229, 1, 0.1f);
    ba = Animation(bt, 0, 0, 32, 64, 16, 0.5f);
    ba2 = Animation(bt2, 0, 0, 32, 64, 16, 0.5f);
    ea = Animation(et, 0, 0, 1100, 1120, 1, 0.1f);

    ba2.sprite.setRotation(180);

    auto player = new Player(pa);
    auto enemy = new Dio(ea, 1000, &entities);
    auto bgrnd = new Entity;
    bgrnd->config(bgra, Vector2f(2.f, 2.f), Vector2f(Width / 2, Height / 2), 0);
    enemy->config(ea, ba2, Vector2f(0.3f, 0.3f), Vector2f(Width / 2, Height / 4), 100);
    player->speed = 500;
    enemy->speed = 125;

    entities.push_back(bgrnd);
    entities.push_back(player);
    entities.push_back(enemy);

    Clock clock;
    auto touhou = new Video;
    music.play();

    while (window.isOpen()) {
        DeltaTime = clock.restart().asSeconds();
        // std::cout << Global::DeltaTime << std::endl;

        Event ev;
        while (window.pollEvent(ev))
            if (ev.type == Event::Closed || Keyboard::isKeyPressed(Keyboard::Escape))
                window.close();

        if (!Paused) {
            if (Keyboard::isKeyPressed(Keyboard::F) && !touhou->isPlaying()) {
                touhou->install("images/touhou", 6572, 30.0003f, "audio/badapple.wav");
                entities.push_back(touhou);
                touhou->start();
            }

            if (Keyboard::isKeyPressed(Keyboard::Space) && player->readyToFire() && !player->markedForNegation) {
                auto nb = new Bullet(ba, player->pos, 10, 850, player);
                nb->config(Bullet::SchlerpType::NONE, player->bulletFunc);
                entities.push_back(nb);
                player->resetFireTimer();
            }

            for (auto it = entities.begin(); it != entities.end();) {
                (*it)->tick();

                if ((*it)->markedForNegation)
                    it = entities.erase(it);

                if (it != entities.end())
                    it++;
            }

            for (auto a : entities)
                for (auto b : entities)
                    a->collision(b);
        } else { // is Paused
	        if (Mouse::isButtonPressed(Mouse::Left)) { // restart
                Paused = false;

                player->reset();
                enemy->reset();

                entities.clear();
                entities.push_back(bgrnd);
                entities.push_back(player);
                entities.push_back(enemy);
                dioHealth.setPosition(Width * 0.8f, Height * 0.75f);
	        }
        }

        if (player->markedForNegation) {
            dioHealth.setString("Game Over!\nClick to retry");
            dioHealth.setPosition(Width / 3, Height / 2);
            Paused = true;
        } else if (enemy->markedForNegation) {
            dioHealth.setString("You Won!\nClick to restart");
            dioHealth.setPosition(Width / 3, Height / 2);
            Paused = true;
        } else {
            dioHealth.setString("Dio: " + std::to_string(enemy->health));
            playerHealth.setString("Health: " + std::to_string(player->health));
        }
        window.clear();

        for (auto e: entities)
            e->draw(window);

        window.draw(dioHealth);
        window.draw(playerHealth);

        window.display();
    }

    return 0;
}
