#ifndef GAME_H
#define GAME_H


#include <list>

#include "definitions.h"
#include "gameobject.h"
#include "player.h"
#include "enemy.h"
#include "shoot.h"
#include "input.h"
#include "audiocontrol.h"
#include "powerup.h"
#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <chrono>
#include "menu.h"

/**
 * @brief Struktur für die Events
 * Enthält affectedObject als Objekt, aus dessen Sicht die Kollision berechnet wurde. affectedObject ist immer ein MovingObject,
 * causingObject kann beides sein.
 * Die Art und Richtung der Kollision werden mit gespeichert.
 * @author Simon, Johann(15.6)
 */
struct collisionStruct {
    GameObject *affectedObject;
    GameObject *causingObject;
    enum collisionDirection direction;
};

/**
 * @brief Game-Klasse
 * Die Game-Klasse bündelt alle Kern-Funktionalitäten des Spiels.
 * Innerhalb der main.cpp wird eine Instanz dieser Klasse angelegt,
 * aus der heraus das gesamte Spiel läuft.
 * Die einzelnen Methoden werden in der game.cpp jeweils erklärt.
 *
 * @lastChanges funtion handleCollisions hinzugefügt
 *
 * @author Simon, Johann
 */
class Game : QObject {
    //Q_OBJECT
public:
    Game(int argc, char *argv[]);
    ~Game();

    /// Startet das die Game-Loop, wird einmalig von main() aufgerufen
    int step();
    /// Startet die Mockup QApplication app
    int run(QApplication& app);

    struct stateStruct gameStats;
    //Liste von Kollisionen
    std::list<struct collisionStruct> collisionsToHandle;
    struct stateStruct states;
    int start();

protected:
    void timerEvent(QTimerEvent *event);

private:
    int getStepIntervall();
    void appendWorldObjects(Player *playerPointer);
    void reduceWorldObjects(Player *playerPointer);
    void evaluateInput();
    void calculateMovement();
    void detectCollision(std::list<GameObject*> *objectsToCalculate);
    void handleCollisions();
    void renderGraphics(std::list<GameObject *> *objectList);
    void endGame();
    bool hurtPlayer(int damage);

    void updateHighScore();

    void colTestLevel();
    void loadLevelFile(QString fileSpecifier);

    void startNewGame();

    /// In der Welt befindliche Objekte
    std::list<GameObject*> worldObjects;
    /// Statische Objekte, die zu Anfang gespawnt werden
    std::list<GameObject*> levelInitial;
    /// Objekte die zur Laufzeit dynamisch gespawnt werden
    std::list<GameObject*> levelSpawn;
    /// Zu löschende Schüsse
    std::list<GameObject*> objectsToDelete;

    /// Liste audioevents mit allen im Step stattfindenden AudioStructs
    std::list<struct audioStruct> audioevents;

    /// Breite der Szene
    int sceneWidth;
    /// Distanz in der Gegner gespawnt werden
    int spawnDistance;
    std::list<struct scoreStruct> scoreList;


    int stepIntervall;
    Player *playerObjPointer;

    /// für das Ausgabefenster QGraphicsView
    QGraphicsScene * levelScene;
    QGraphicsView * window;
    QGraphicsPixmapItem * background;

    /// aktueller Spielzustand (running, menuStart, menuEnd)
    gameState state = gameMenuStart;

    /// Zeiger auf QApplication
    QApplication *appPointer;
    /// für Zeitmessung
    std::chrono::high_resolution_clock::time_point letzterAufruf;
    /// Erstelle Input Objekt zum Aufzeichnen der Keyboard Inputs
    Input *keyInput = new Input();
    /// Erstelle Audiocontrol Objekt zum Erzeugen der Soundausgabe
    AudioControl *audioOutput = new AudioControl();
    /// Menüs
    Menu *menuStart;
    Menu *menuEnd;
    /// zur Unterscheidung und Identifizierung der Menü-Einträge
    enum menuIds {
        menuId_StartGame, menuId_EndGame, menuId_Resume, menuId_Highscore, menuId_Credits, menuId_GotoStartMenu
    };

    /// stepCount wird mit jedem Step um ein erhöht
    /// Auslesen der vergangenen Zeit: stepCount * getStepIntervall()
    int stepCount = 0;
};

#endif // GAME_H

