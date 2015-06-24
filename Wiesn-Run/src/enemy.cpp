#include "enemy.h"

/// Class Enemy
/// lastUpdate:  update() 10.6 Johann

/**
 * @brief Konstruktor für ein Enemy-Objekt
 * @param posX      : X-Position
 * @param posY      : Y-Position
 * @param speedX    : Geschwindigkeit in X-Richtung
 *
 * @todo Skalieren der Werte und fireCooldown erhöhen
 */
Enemy::Enemy(int posX, int posY, int speedX) : MovingObject(posX, posY, enemy, speedX, -5) {
    health = 1;
    ///Fire Cooldown für debug zwecke niedrig
    fireCooldown = 2;
    inflictedDamage = 1;
    death = false;

    DeathCooldown = frameRate;

    //Grafik - Enemygrafik initialisieren
    setPixmap(QPixmap(":/images/images/enemy.png"));
    setPos(getPosX() - getLength()*0.5, -getPosY() + 548);
}

Enemy::~Enemy() {

}

/**
 * @brief Enemy::getHealth
 * Gibt Lebensstand zurück
 *
 * @return : Lebensstand
 */
int Enemy::getHealth() const {
    return health;
}

/**
 * @brief Enemy::setHealth
 * Lebensstand wird gesetzt
 *
 * @param health : Lebensstand
 */
void Enemy::setHealth(int health) {
    this->health = health;
}

bool Enemy::receiveDamage(int damage) {
    health = health - damage;
    return (!(health > 0));
}

/**
 * @brief Enemy::getInflictedDamage
 * gibt Schaden zurück, den der gegner zufügt
 *
 * @return : Schaden
 */
int Enemy::getInflictedDamage() const {
    return inflictedDamage;
}

/**
 * @brief Enemy::getFireCooldown
 * @return fireCooldown
 */
int Enemy::getFireCooldown() const{
    return fireCooldown;
}

/**
 * @brief Enemy::getDeath
 * Gibt an ob der Gegner Tot ist
 *
 * @return : Zustand - TOT
 */
bool Enemy::getDeath() const {
    return death;
}

/**
 * @brief Enemy::setDeath
 * Zustand-TOT wird gesetzt
 *
 * @param death : Zustand-TOT
 */
void Enemy::setDeath(bool death) {
    this->death = death;
}

/**
 * @brief Enemy::getDeathCooldown
 * @return deathCooldown
 */
int Enemy::getDeathCooldown() const {
    return DeathCooldown;
}

/**
 * @brief Enemy::update
 * führt Bewegungen des Gegners aus
 */
void Enemy::update() {
    if (death) {
        DeathCooldown = DeathCooldown - 1;
    } else {
        //Bewegung durchführen
        updatePosition();

        //Grafik - Bewegung anzeigen
        setPos(getPosX() - 0.5*getLength(), -getPosY() + 548);

        if (fireCooldown == 0) {
            fireCooldown = fireRate;
        } else {
            fireCooldown = fireCooldown - 1;
        }
    }
}
