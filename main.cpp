#include <iostream>
#include <array>
#include <chrono>
#include <thread>
#include <tuple>

#include <SFML/Graphics.hpp>

#include <Helper.h>

//////////////////////////////////////////////////////////////////////
/// NOTE: this include is needed for environment-specific fixes     //
/// You can remove this include and the call from main              //
/// if you have tested on all environments, and it works without it //
#include "env_fixes.h"                                              //
//////////////////////////////////////////////////////////////////////

#define TILE_SIZE 24.0f
//////////////////////////////////////////////////////////////////////
/// This class is used to test that the memory leak checks work as expected even when using a GUI
class SomeClass {
public:
    explicit SomeClass(int) {}
};

SomeClass *getC() {
    return new SomeClass{2};
}
//////////////////////////////////////////////////////////////////////

class Rectangle {
private:
    int _x, _y, _w, _h;
public:
    void move(int offX, int offY) {
        _x += offX;
        _y += offY;
    }

    std::tuple<int, int> lowerBounds() const {
        return { std::min(_x, _x + _w) * TILE_SIZE, std::min(_y, _y + _h) * TILE_SIZE };
    }
    std::tuple<int, int> upperBounds() const {
        return { std::max(_x, _x + _w) * TILE_SIZE, std::max(_y, _y + _h) * TILE_SIZE };
    }
    Rectangle(int x, int y, int w, int h) : _x(x), _y(y), _w(w), _h(h) {};
    Rectangle(const Rectangle &r) : _x(r._x), _y(r._y), _w(r._w), _h(r._h) {};
    Rectangle& operator=(const Rectangle& r) {
        _x = r._x;
        _y = r._y;
        _w = r._w;
        _h = r._h;
        return *this;
    }
    ~Rectangle() {
        std::cout << "Destroying Rectangle@" << this << std::endl;
    }
    friend std::ostream& operator<<(std::ostream& os, const Rectangle& rt);
};

std::ostream &operator<<(std::ostream &os, const Rectangle &rt) {
    os << "Rectangle(" << rt._x << ", " << rt._y << ", " << rt._w << ", " << rt._h << ")";
    return os;
}

class Cursor : public sf::Drawable {
private:
    sf::Text _curs_text;
    sf::RectangleShape _sel_shape;
    Rectangle _rt;

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
        target.draw(_sel_shape);
        target.draw(_curs_text);
    }

public:
    void update(int offX, int offY) {
        _rt.move(offX, offY);
        this->_curs_text.move(offX * TILE_SIZE, offY * TILE_SIZE);
    }
    Cursor(const sf::Font& font, int x, int y) : _rt(x, y, 1, 1) {
        this->_curs_text.setCharacterSize(TILE_SIZE);
        this->_curs_text.setString("@");
        this->_curs_text.setFont(font);
        this->_curs_text.setPosition(x * TILE_SIZE, y * TILE_SIZE);
    }
};

class Orca {

};
int main() {
    ////////////////////////////////////////////////////////////////////////
    /// NOTE: this function call is needed for environment-specific fixes //
    init_threads();                                                       //
    ////////////////////////////////////////////////////////////////////////
    ///

    ///////////////////////////////////////////////////////////////////////////
    ///                Exemplu de utilizare cod generat                     ///
    ///////////////////////////////////////////////////////////////////////////
    Helper helper;
    helper.help();
    ///////////////////////////////////////////////////////////////////////////

    SomeClass *c = getC();
    std::cout << c << "\n";
    delete c;

    Rectangle rt1 = Rectangle(20, 20, -1, 20);
    Rectangle rt2(rt1);

    std::cout << rt1 << std::endl;
    std::cout << rt2 << std::endl;

    sf::Font font;
    if (!font.loadFromFile("TerminusTTF.ttf")) {
        std::cerr << "Could not load font!" << std::endl;
        return 1;
    }

    Cursor cursor = Cursor(font, 8, 8);

    sf::RenderWindow window;
    ///////////////////////////////////////////////////////////////////////////
    /// NOTE: sync with env variable APP_WINDOW from .github/workflows/cmake.yml:31
    window.create(sf::VideoMode({800, 700}), "My Window", sf::Style::Default);
    ///////////////////////////////////////////////////////////////////////////
    //
    ///////////////////////////////////////////////////////////////////////////
    /// NOTE: mandatory use one of vsync or FPS limit (not both)            ///
    /// This is needed so we do not burn the GPU                            ///
    window.setVerticalSyncEnabled(true);                            ///
    /// window.setFramerateLimit(60);                                       ///
    ///////////////////////////////////////////////////////////////////////////

    while(window.isOpen()) {
        bool shouldExit = false;
        sf::Event e{};

        int offX = 0, offY = 0;

        while(window.pollEvent(e)) {
            switch(e.type) {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::Resized:
                std::cout << "New width: " << window.getSize().x << '\n'
                          << "New height: " << window.getSize().y << '\n';
                break;
            case sf::Event::KeyPressed:
                std::cout << "Received key " << (e.key.code == sf::Keyboard::X ? "X" : "(other)") << "\n";
                if(e.key.code == sf::Keyboard::Up) offY = -1;
                if(e.key.code == sf::Keyboard::Down) offY = 1;
                if(e.key.code == sf::Keyboard::Left) offX = -1;
                if(e.key.code == sf::Keyboard::Right) offX = 1;
                if(e.key.code == sf::Keyboard::Escape)
                    shouldExit = true;
                break;
            default:
                break;
            }
        }
        if(shouldExit) {
            window.close();
            break;
        }
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(66ms);

        cursor.update(offX, offY);
        window.clear();
        window.draw(cursor);
        window.display();
    }
    return 0;
}
