#include <iostream>
#include <array>
#include <chrono>
#include <thread>
#include <tuple>

#include <SFML/Graphics.hpp>
#include <utility>

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

    void resize(int offW, int offH) {
        _w += offW;
        if(_w == 0) _w += offW;
        _h += offH;
        if(_h == 0) _h += offH;
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

class ResourcesStore {
private:
    sf::Font _font;
public:
    bool loadResources() {
        if (!_font.loadFromFile("TerminusTTF.ttf")) {
            std::cerr << "Could not load font!" << std::endl;
                return false;
        }

        return true;
    }

    sf::Font& getFont() {
        return _font;
    }
};

class Cursor : public sf::Drawable {
private:
    ResourcesStore _rs;
    sf::Text _curs_text;
    sf::RectangleShape _sel_shape;
    Rectangle _rt;

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
        (void)states;

        target.draw(_sel_shape);
        target.draw(_curs_text);
    }

public:
    void update(int offX, int offY, int offW, int offH) {
        _rt.move(offX, offY);
        _rt.resize(offW, offH);

        this->_curs_text.move(offX * TILE_SIZE, offY * TILE_SIZE);

        int x1, y1, x2, y2;
        std::tie(x1, y1) = _rt.lowerBounds();
        std::tie(x2, y2) = _rt.upperBounds();

        this->_sel_shape.setPosition(x1, y1);
        this->_sel_shape.setSize(sf::Vector2f(x2, y2) - sf::Vector2f(x1, y1));
    }
    Cursor(ResourcesStore rs, int x, int y) : _rs(std::move(rs)), _rt(x, y, 1, 1) {
        this->_curs_text.setCharacterSize(TILE_SIZE);
        this->_curs_text.setFillColor(sf::Color(0, 255, 0));
        this->_curs_text.setString("@");
        this->_curs_text.setFont(_rs.getFont());
        this->_curs_text.setPosition(x * TILE_SIZE, y * TILE_SIZE);
    }
    friend std::ostream& operator<<(std::ostream& os, const Cursor& curs);
};

std::ostream &operator<<(std::ostream &os, const Cursor &curs) {
    os << "Cursor: " << curs._rt;
    return os;
}

class Orca {
private:
    ResourcesStore _rs;
    Cursor _curs;
    sf::RenderWindow _window;
    int _w, _h;
public:
    friend std::ostream& operator<<(std::ostream& os, const Orca& orca);

    Orca(ResourcesStore rs, int w, int h) : _rs(std::move(rs)), _curs(Cursor(_rs, 8, 8)), _w(w), _h(h) {
        _window.create(sf::VideoMode({700, 800}), "My Window", sf::Style::Default);
        _window.setVerticalSyncEnabled(true);
    }

    void mainLoop() {
        while(_window.isOpen()) {
            bool shouldExit = false;
            sf::Event e{};

            int offX = 0, offY = 0, offW = 0, offH = 0;

            while(_window.pollEvent(e)) {
                switch(e.type) {
                    case sf::Event::Closed:
                        _window.close();
                        break;
                    case sf::Event::Resized:
                        std::cout << "New width: " << _window.getSize().x << '\n'
                                  << "New height: " << _window.getSize().y << '\n';
                        break;
                    case sf::Event::KeyPressed:
                        std::cout << _curs << std::endl;

                        std::cout << "Received key " << (e.key.code == sf::Keyboard::X ? "X" : "(other)") << "\n";
                        if(e.key.code == sf::Keyboard::Up) offY = -1;
                        if(e.key.code == sf::Keyboard::Down) offY = 1;
                        if(e.key.code == sf::Keyboard::Left) offX = -1;
                        if(e.key.code == sf::Keyboard::Right) offX = 1;
                        if(e.key.code == sf::Keyboard::W) offH = -1;
                        if(e.key.code == sf::Keyboard::S) offH = 1;
                        if(e.key.code == sf::Keyboard::A) offW = -1;
                        if(e.key.code == sf::Keyboard::D) offW = 1;
                        if(e.key.code == sf::Keyboard::Escape)
                            shouldExit = true;
                        break;
                    default:
                        break;
                }
            }
            if(shouldExit) {
                _window.close();
                break;
            }
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(66ms);

            _curs.update(offX, offY, offW, offH);
            _window.clear();
            _window.draw(_curs);
            _window.display();
        }
    }
};

std::ostream &operator<<(std::ostream &os, const Orca &orca) {
    os << "Orca(" << orca._w << ", " << orca._h << ")";
    return os;
}

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

    ResourcesStore rs = ResourcesStore();
    if(!rs.loadResources()){
        return 1;
    }

    Orca orca = Orca(rs, 80, 80);
    orca.mainLoop();

    return 0;
}
