#pragma once

#include <QVector>
#include "api/ApiModels.h"

// Provides realistic 2026 World Cup demo data so the app works out of the
// box without an API key.  Call DemoData::teams() and DemoData::playersForTeam()
// to obtain the seed data; DatabaseManager uses this when the cache is empty.

namespace DemoData {

// ── Helper ────────────────────────────────────────────────────────────────────

inline Models::Player makePlayer(
    int id, int teamId, const QString &name, const QString &first,
    const QString &last, int age, const QString &nat,
    Models::Position pos, int jersey,
    double passAcc, int goals, int assists,
    int tackles, int intercepts,
    double savePct, int cleanSheets,
    double shotsOnPct, double dribblesSuccPct,
    int keyPasses, int aerialWon)
{
    Models::Player p;
    p.id           = id;
    p.teamId       = teamId;
    p.name         = name;
    p.firstname    = first;
    p.lastname     = last;
    p.age          = age;
    p.nationality  = nat;
    p.position     = pos;
    p.jerseyNumber = jersey;

    auto &s = p.stats;
    s.passAccuracy       = passAcc;
    s.goals              = goals;
    s.assists            = assists;
    s.tackles            = tackles;
    s.interceptions      = intercepts;
    s.savesPercentage    = savePct;
    s.cleanSheets        = cleanSheets;
    s.shotsOnTargetPct   = shotsOnPct;
    s.dribbleSuccessPct  = dribblesSuccPct;
    s.keyPasses          = keyPasses;
    s.aerialDuelsWon     = aerialWon;
    s.appearances        = 10;
    s.saves              = static_cast<int>(savePct * 0.6);
    s.goalsConceded      = static_cast<int>(savePct * 0.15);
    return p;
}

// ── Brazil ────────────────────────────────────────────────────────────────────

inline Models::Team brazil()
{
    Models::Team t;
    t.id = 6; t.name = "Brazil"; t.shortName = "BRA"; t.country = "Brazil";
    using P = Models::Position;
    const int tid = 6;
    t.players = {
        makePlayer(601,tid,"Ederson Moraes","Ederson","Moraes",31,"Brazil",P::Goalkeeper,1,  72,0,0,0,0, 74,14, 0, 0,0,2),
        makePlayer(602,tid,"Alisson Becker","Alisson","Becker",32,"Brazil",P::Goalkeeper,12, 68,0,0,0,0, 78,18, 0, 0,0,1),
        makePlayer(603,tid,"Danilo","Danilo","Luiz",33,"Brazil",P::Defender,2,            81,2,3,85,42, 0,0, 0,55,8,38),
        makePlayer(604,tid,"Marquinhos","Marquinhos","",30,"Brazil",P::Defender,4,        84,1,2,90,55, 0,0, 0,52,6,72),
        makePlayer(605,tid,"Gabriel Magalhães","Gabriel","Magalhães",26,"Brazil",P::Defender,3, 80,3,1,88,50, 0,0, 0,45,4,80),
        makePlayer(606,tid,"Alex Telles","Alex","Telles",32,"Brazil",P::Defender,6,       78,2,4,70,38, 0,0, 0,60,7,40),
        makePlayer(607,tid,"Éder Militão","Éder","Militão",26,"Brazil",P::Defender,5,     83,1,1,92,58, 0,0, 0,48,5,78),
        makePlayer(608,tid,"Casemiro","Casemiro","",32,"Brazil",P::Midfielder,5,          80,5,4,95,65, 0,0, 0,45,22,65),
        makePlayer(609,tid,"Lucas Paquetá","Lucas","Paquetá",27,"Brazil",P::Midfielder,10,88,7,8,42,25, 0,0, 0,68,40,28),
        makePlayer(610,tid,"Gerson","Gerson","",27,"Brazil",P::Midfielder,8,              83,4,6,65,40, 0,0, 0,62,32,35),
        makePlayer(611,tid,"Bruno Guimarães","Bruno","Guimarães",27,"Brazil",P::Midfielder,18,87,5,7,75,48, 0,0, 0,65,38,42),
        makePlayer(612,tid,"Vinicius Jr","Vinicius","Junior",24,"Brazil",P::Forward,20,   72,22,9,18,8, 0,0,68,78,28,15),
        makePlayer(613,tid,"Rodrygo","Rodrygo","Goes",23,"Brazil",P::Forward,11,          70,15,8,12,5, 0,0,62,72,24,12),
        makePlayer(614,tid,"Gabriel Martinelli","Gabriel","Martinelli",23,"Brazil",P::Forward,7,68,14,7,15,6, 0,0,60,70,20,10),
        makePlayer(615,tid,"Richarlison","Richarlison","",27,"Brazil",P::Forward,9,       65,18,5,10,4, 0,0,65,60,15,25),
        makePlayer(616,tid,"Gabriel Jesus","Gabriel","Jesus",27,"Brazil",P::Forward,19,   68,12,9,14,5, 0,0,58,68,22,18),
        makePlayer(617,tid,"Endrick","Endrick","",18,"Brazil",P::Forward,16,              60,10,4, 8,3, 0,0,55,65,12, 8),
        makePlayer(618,tid,"Raphinha","Raphinha","",28,"Brazil",P::Forward,10,            71,16,12,12,5, 0,0,62,74,30,12),
    };
    return t;
}

// ── France ────────────────────────────────────────────────────────────────────

inline Models::Team france()
{
    Models::Team t;
    t.id = 2; t.name = "France"; t.shortName = "FRA"; t.country = "France";
    using P = Models::Position;
    const int tid = 2;
    t.players = {
        makePlayer(201,tid,"Mike Maignan","Mike","Maignan",29,"France",P::Goalkeeper,16,  70,0,0,0,0, 76,15, 0, 0,0,2),
        makePlayer(202,tid,"Alphonse Areola","Alphonse","Areola",31,"France",P::Goalkeeper,23,65,0,0,0,0, 68,10, 0, 0,0,1),
        makePlayer(203,tid,"Benjamin Pavard","Benjamin","Pavard",28,"France",P::Defender,5, 82,2,3,80,45, 0,0, 0,55,9,42),
        makePlayer(204,tid,"Raphaël Varane","Raphaël","Varane",31,"France",P::Defender,4,  84,1,1,88,52, 0,0, 0,48,5,80),
        makePlayer(205,tid,"Dayot Upamecano","Dayot","Upamecano",26,"France",P::Defender,2, 80,1,2,85,50, 0,0, 0,50,5,75),
        makePlayer(206,tid,"Théo Hernandez","Théo","Hernandez",27,"France",P::Defender,22, 78,3,5,72,38, 0,0, 0,65,10,35),
        makePlayer(207,tid,"Jules Koundé","Jules","Koundé",25,"France",P::Defender,5,      83,2,3,87,55, 0,0, 0,60,8,55),
        makePlayer(208,tid,"N'Golo Kanté","N'Golo","Kanté",33,"France",P::Midfielder,13,   82,3,5,105,78, 0,0, 0,62,25,48),
        makePlayer(209,tid,"Aurélien Tchouaméni","Aurélien","Tchouaméni",24,"France",P::Midfielder,8, 84,4,4,88,60, 0,0, 0,58,28,52),
        makePlayer(210,tid,"Antoine Griezmann","Antoine","Griezmann",33,"France",P::Midfielder,7, 87,10,12,40,22, 0,0, 0,65,48,25),
        makePlayer(211,tid,"Adrien Rabiot","Adrien","Rabiot",29,"France",P::Midfielder,14, 83,6,5,70,42, 0,0, 0,60,32,40),
        makePlayer(212,tid,"Kylian Mbappé","Kylian","Mbappé",26,"France",P::Forward,10,    72,36,18,15,5, 0,0,72,82,32,15),
        makePlayer(213,tid,"Ousmane Dembélé","Ousmane","Dembélé",27,"France",P::Forward,11, 68,14,15,10,4, 0,0,62,78,28,10),
        makePlayer(214,tid,"Marcus Thuram","Marcus","Thuram",27,"France",P::Forward,9,      65,18,7,12,5, 0,0,65,62,18,20),
        makePlayer(215,tid,"Olivier Giroud","Olivier","Giroud",38,"France",P::Forward,18,   70,10,4,10,4, 0,0,60,45,14,45),
        makePlayer(216,tid,"Kingsley Coman","Kingsley","Coman",28,"France",P::Forward,20,   70,10,12,10,4, 0,0,60,72,25,12),
        makePlayer(217,tid,"Randal Kolo Muani","Randal","Kolo Muani",26,"France",P::Forward,19, 65,12,8,10,4, 0,0,60,65,18,15),
    };
    return t;
}

// ── England ───────────────────────────────────────────────────────────────────

inline Models::Team england()
{
    Models::Team t;
    t.id = 10; t.name = "England"; t.shortName = "ENG"; t.country = "England";
    using P = Models::Position;
    const int tid = 10;
    t.players = {
        makePlayer(1001,tid,"Jordan Pickford","Jordan","Pickford",30,"England",P::Goalkeeper,1,  68,0,0,0,0, 72,13, 0, 0,0,2),
        makePlayer(1002,tid,"Aaron Ramsdale","Aaron","Ramsdale",26,"England",P::Goalkeeper,12,   65,0,0,0,0, 68, 9, 0, 0,0,1),
        makePlayer(1003,tid,"Reece James","Reece","James",25,"England",P::Defender,24,            83,3,6,80,45, 0,0, 0,62,12,40),
        makePlayer(1004,tid,"Harry Maguire","Harry","Maguire",31,"England",P::Defender,5,         78,2,1,85,50, 0,0, 0,42,5,82),
        makePlayer(1005,tid,"John Stones","John","Stones",30,"England",P::Defender,5,             85,2,3,88,55, 0,0, 0,50,8,72),
        makePlayer(1006,tid,"Luke Shaw","Luke","Shaw",29,"England",P::Defender,23,                79,2,5,75,40, 0,0, 0,60,10,38),
        makePlayer(1007,tid,"Trent Alexander-Arnold","Trent","Alexander-Arnold",26,"England",P::Defender,66, 88,4,12,65,35, 0,0, 0,65,20,30),
        makePlayer(1008,tid,"Declan Rice","Declan","Rice",26,"England",P::Midfielder,4,           85,5,6,100,68, 0,0, 0,60,30,52),
        makePlayer(1009,tid,"Jude Bellingham","Jude","Bellingham",21,"England",P::Midfielder,22,  88,14,8,55,35, 0,0, 0,72,45,35),
        makePlayer(1010,tid,"Phil Foden","Phil","Foden",24,"England",P::Midfielder,47,            89,12,10,35,18, 0,0, 0,78,48,20),
        makePlayer(1011,tid,"Mason Mount","Mason","Mount",25,"England",P::Midfielder,19,          86,8,8,45,25, 0,0, 0,70,40,22),
        makePlayer(1012,tid,"Harry Kane","Harry","Kane",31,"England",P::Forward,9,                75,34,12,12,5, 0,0,72,55,25,38),
        makePlayer(1013,tid,"Bukayo Saka","Bukayo","Saka",23,"England",P::Forward,7,              78,15,14,14,6, 0,0,64,72,28,12),
        makePlayer(1014,tid,"Raheem Sterling","Raheem","Sterling",30,"England",P::Forward,10,      72,12,10,12,5, 0,0,60,72,22,10),
        makePlayer(1015,tid,"Marcus Rashford","Marcus","Rashford",27,"England",P::Forward,11,      70,14,8,10,4, 0,0,62,70,20,12),
        makePlayer(1016,tid,"Jack Grealish","Jack","Grealish",30,"England",P::Forward,7,           75,8,12,10,4, 0,0,55,75,30,10),
        makePlayer(1017,tid,"Ollie Watkins","Ollie","Watkins",30,"England",P::Forward,11,          68,16,6, 8,3, 0,0,65,58,16,18),
    };
    return t;
}

// ── Argentina ─────────────────────────────────────────────────────────────────

inline Models::Team argentina()
{
    Models::Team t;
    t.id = 26; t.name = "Argentina"; t.shortName = "ARG"; t.country = "Argentina";
    using P = Models::Position;
    const int tid = 26;
    t.players = {
        makePlayer(2601,tid,"Emiliano Martínez","Emiliano","Martínez",32,"Argentina",P::Goalkeeper,23, 72,0,0,0,0,80,18, 0,0,0,2),
        makePlayer(2602,tid,"Juan Musso","Juan","Musso",30,"Argentina",P::Goalkeeper,1,              68,0,0,0,0,72,12, 0,0,0,1),
        makePlayer(2603,tid,"Nahuel Molina","Nahuel","Molina",26,"Argentina",P::Defender,26,          82,3,5,78,42, 0,0, 0,62,10,38),
        makePlayer(2604,tid,"Cristian Romero","Cristian","Romero",26,"Argentina",P::Defender,13,      82,1,1,95,60, 0,0, 0,50,6,78),
        makePlayer(2605,tid,"Nicolás Otamendi","Nicolás","Otamendi",37,"Argentina",P::Defender,19,    78,2,1,92,58, 0,0, 0,45,5,80),
        makePlayer(2606,tid,"Marcos Acuña","Marcos","Acuña",32,"Argentina",P::Defender,8,             78,2,4,80,45, 0,0, 0,60,10,42),
        makePlayer(2607,tid,"Lisandro Martínez","Lisandro","Martínez",26,"Argentina",P::Defender,25,  83,1,2,90,56, 0,0, 0,52,6,76),
        makePlayer(2608,tid,"Rodrigo De Paul","Rodrigo","De Paul",30,"Argentina",P::Midfielder,7,     85,6,10,80,52, 0,0, 0,68,40,38),
        makePlayer(2609,tid,"Alexis Mac Allister","Alexis","Mac Allister",25,"Argentina",P::Midfielder,20, 87,8,8,72,48, 0,0, 0,65,38,35),
        makePlayer(2610,tid,"Enzo Fernández","Enzo","Fernández",24,"Argentina",P::Midfielder,24,      86,5,7,78,52, 0,0, 0,62,36,40),
        makePlayer(2611,tid,"Leandro Paredes","Leandro","Paredes",30,"Argentina",P::Midfielder,5,     85,3,5,68,42, 0,0, 0,58,30,30),
        makePlayer(2612,tid,"Lionel Messi","Lionel","Messi",37,"Argentina",P::Forward,10,             92,18,18, 8,5, 0,0,65,80,55,15),
        makePlayer(2613,tid,"Lautaro Martínez","Lautaro","Martínez",27,"Argentina",P::Forward,22,     72,28,10,12,5, 0,0,70,62,20,28),
        makePlayer(2614,tid,"Julián Álvarez","Julián","Álvarez",24,"Argentina",P::Forward,9,           70,20,12,14,6, 0,0,65,68,22,18),
        makePlayer(2615,tid,"Nicolás González","Nicolás","González",26,"Argentina",P::Forward,11,      68,12,6,10,4, 0,0,60,65,18,12),
        makePlayer(2616,tid,"Alejandro Garnacho","Alejandro","Garnacho",20,"Argentina",P::Forward,11,  65,10,8,10,4, 0,0,58,72,20,10),
        makePlayer(2617,tid,"Ángel Di María","Ángel","Di María",36,"Argentina",P::Forward,11,          78,10,14, 8,4, 0,0,55,70,30,10),
    };
    return t;
}

// ── Germany ───────────────────────────────────────────────────────────────────

inline Models::Team germany()
{
    Models::Team t;
    t.id = 25; t.name = "Germany"; t.shortName = "GER"; t.country = "Germany";
    using P = Models::Position;
    const int tid = 25;
    t.players = {
        makePlayer(2501,tid,"Manuel Neuer","Manuel","Neuer",39,"Germany",P::Goalkeeper,1,  75,0,0,0,0,72,15, 0,0,0,3),
        makePlayer(2502,tid,"Marc-André ter Stegen","Marc-André","ter Stegen",32,"Germany",P::Goalkeeper,22,78,0,0,0,0,76,16, 0,0,0,2),
        makePlayer(2503,tid,"Joshua Kimmich","Joshua","Kimmich",29,"Germany",P::Defender,6, 90,4,12,88,55, 0,0, 0,68,18,42),
        makePlayer(2504,tid,"Antonio Rüdiger","Antonio","Rüdiger",31,"Germany",P::Defender,2, 80,2,1,95,62, 0,0, 0,50,5,82),
        makePlayer(2505,tid,"Niklas Süle","Niklas","Süle",28,"Germany",P::Defender,5,        80,1,2,90,55, 0,0, 0,48,5,78),
        makePlayer(2506,tid,"David Raum","David","Raum",26,"Germany",P::Defender,3,           78,2,6,72,40, 0,0, 0,62,12,35),
        makePlayer(2507,tid,"Benjamin Henrichs","Benjamin","Henrichs",27,"Germany",P::Defender,23, 78,2,4,75,42, 0,0, 0,60,10,40),
        makePlayer(2508,tid,"Leon Goretzka","Leon","Goretzka",29,"Germany",P::Midfielder,8,   86,8,8,85,52, 0,0, 0,62,38,50),
        makePlayer(2509,tid,"İlkay Gündoğan","İlkay","Gündoğan",34,"Germany",P::Midfielder,21, 90,10,10,70,45, 0,0, 0,70,45,30),
        makePlayer(2510,tid,"Florian Wirtz","Florian","Wirtz",21,"Germany",P::Midfielder,10,  91,14,16,40,22, 0,0, 0,80,50,18),
        makePlayer(2511,tid,"Kai Havertz","Kai","Havertz",25,"Germany",P::Midfielder,7,        84,10,8,45,25, 0,0, 0,68,38,28),
        makePlayer(2512,tid,"Leroy Sané","Leroy","Sané",28,"Germany",P::Forward,19,             72,15,14,12,5, 0,0,62,78,28,10),
        makePlayer(2513,tid,"Jamal Musiala","Jamal","Musiala",21,"Germany",P::Forward,14,       78,16,12,14,6, 0,0,64,82,32,12),
        makePlayer(2514,tid,"Niclas Füllkrug","Niclas","Füllkrug",31,"Germany",P::Forward,9,    65,20,6,10,4, 0,0,68,50,15,35),
        makePlayer(2515,tid,"Thomas Müller","Thomas","Müller",35,"Germany",P::Forward,13,       84,12,16,15,6, 0,0,58,65,42,18),
        makePlayer(2516,tid,"Serge Gnabry","Serge","Gnabry",29,"Germany",P::Forward,10,          70,12,10,10,4, 0,0,60,72,22,10),
        makePlayer(2517,tid,"Deniz Undav","Deniz","Undav",27,"Germany",P::Forward,21,             65,14,6, 8,3, 0,0,62,58,16,18),
    };
    return t;
}

// ── Spain ─────────────────────────────────────────────────────────────────────

inline Models::Team spain()
{
    Models::Team t;
    t.id = 9; t.name = "Spain"; t.shortName = "ESP"; t.country = "Spain";
    using P = Models::Position;
    const int tid = 9;
    t.players = {
        makePlayer(901,tid,"Unai Simón","Unai","Simón",27,"Spain",P::Goalkeeper,1,  74,0,0,0,0,74,14, 0,0,0,2),
        makePlayer(902,tid,"David Raya","David","Raya",29,"Spain",P::Goalkeeper,25,  72,0,0,0,0,76,16, 0,0,0,2),
        makePlayer(903,tid,"Dani Carvajal","Dani","Carvajal",32,"Spain",P::Defender,2, 84,2,5,80,45, 0,0, 0,62,12,40),
        makePlayer(904,tid,"Aymeric Laporte","Aymeric","Laporte",30,"Spain",P::Defender,14, 88,2,4,90,55, 0,0, 0,55,8,72),
        makePlayer(905,tid,"Pau Cubarsí","Pau","Cubarsí",18,"Spain",P::Defender,24,    85,1,3,88,52, 0,0, 0,58,7,70),
        makePlayer(906,tid,"Marc Cucurella","Marc","Cucurella",26,"Spain",P::Defender,24, 82,1,4,78,45, 0,0, 0,62,10,38),
        makePlayer(907,tid,"Alejandro Balde","Alejandro","Balde",21,"Spain",P::Defender,3,  80,2,6,72,40, 0,0, 0,68,12,32),
        makePlayer(908,tid,"Rodri","Rodri","",28,"Spain",P::Midfielder,16,              91,5,8,98,70, 0,0, 0,65,38,55),
        makePlayer(909,tid,"Pedri","Pedri","",22,"Spain",P::Midfielder,8,               92,8,12,55,35, 0,0, 0,82,52,22),
        makePlayer(910,tid,"Gavi","Gavi","",20,"Spain",P::Midfielder,6,                  88,6,10,65,45, 0,0, 0,80,45,28),
        makePlayer(911,tid,"Fabián Ruiz","Fabián","Ruiz",28,"Spain",P::Midfielder,8,      87,7,9,62,40, 0,0, 0,72,42,28),
        makePlayer(912,tid,"Dani Olmo","Dani","Olmo",26,"Spain",P::Forward,10,            85,10,10,30,15, 0,0,62,75,38,15),
        makePlayer(913,tid,"Lamine Yamal","Lamine","Yamal",17,"Spain",P::Forward,19,       78,12,14,12,5, 0,0,65,80,35,10),
        makePlayer(914,tid,"Nico Williams","Nico","Williams",22,"Spain",P::Forward,11,     75,14,12,12,5, 0,0,64,78,30,10),
        makePlayer(915,tid,"Álvaro Morata","Álvaro","Morata",32,"Spain",P::Forward,7,      70,16,6,10,4, 0,0,65,55,18,32),
        makePlayer(916,tid,"Mikel Oyarzabal","Mikel","Oyarzabal",27,"Spain",P::Forward,7,  78,12,10,10,4, 0,0,62,70,28,18),
        makePlayer(917,tid,"Ferran Torres","Ferran","Torres",25,"Spain",P::Forward,11,     70,10,8,10,4, 0,0,60,65,20,12),
    };
    return t;
}

// ── Morocco ───────────────────────────────────────────────────────────────────

inline Models::Team morocco()
{
    Models::Team t;
    t.id = 31; t.name = "Morocco"; t.shortName = "MAR"; t.country = "Morocco";
    using P = Models::Position;
    const int tid = 31;
    t.players = {
        // Goalkeepers
        makePlayer(3101,tid,"Yassine Bounou","Yassine","Bounou",33,"Morocco",P::Goalkeeper,1,   74,0,0,0,0,76,16, 0,0,0,3),
        makePlayer(3102,tid,"Munir Mohamedi","Munir","Mohamedi",33,"Morocco",P::Goalkeeper,16,  65,0,0,0,0,68, 8, 0,0,0,1),
        makePlayer(3103,tid,"Anas Zniti","Anas","Zniti",34,"Morocco",P::Goalkeeper,23,          62,0,0,0,0,65, 6, 0,0,0,1),

        // Defenders
        makePlayer(3104,tid,"Achraf Hakimi","Achraf","Hakimi",26,"Morocco",P::Defender,2,       87,5,10,78,42, 0,0, 0,72,18,35),
        makePlayer(3105,tid,"Noussair Mazraoui","Noussair","Mazraoui",27,"Morocco",P::Defender,12, 83,2,5,80,45, 0,0, 0,65,12,38),
        makePlayer(3106,tid,"Romain Saïss","Romain","Saïss",34,"Morocco",P::Defender,5,         80,2,2,90,55, 0,0, 0,48,5,80),
        makePlayer(3107,tid,"Nayef Aguerd","Nayef","Aguerd",28,"Morocco",P::Defender,6,          82,2,2,88,55, 0,0, 0,50,6,78),
        makePlayer(3108,tid,"Jawad El Yamiq","Jawad","El Yamiq",32,"Morocco",P::Defender,3,      78,1,1,85,50, 0,0, 0,45,4,72),
        makePlayer(3109,tid,"Yahya Attiat-Allah","Yahya","Attiat-Allah",31,"Morocco",P::Defender,22, 76,1,3,75,40, 0,0, 0,58,8,35),
        makePlayer(3110,tid,"Achraf Dari","Achraf","Dari",26,"Morocco",P::Defender,4,            79,1,1,82,48, 0,0, 0,50,5,70),

        // Midfielders
        makePlayer(3111,tid,"Sofyan Amrabat","Sofyan","Amrabat",28,"Morocco",P::Midfielder,4,    84,3,5,105,72, 0,0, 0,60,28,55),
        makePlayer(3112,tid,"Azzedine Ounahi","Azzedine","Ounahi",24,"Morocco",P::Midfielder,8,  85,5,7,70,45, 0,0, 0,70,38,30),
        makePlayer(3113,tid,"Selim Amallah","Selim","Amallah",27,"Morocco",P::Midfielder,18,     82,4,6,65,40, 0,0, 0,65,32,28),
        makePlayer(3114,tid,"Ilias Chair","Ilias","Chair",27,"Morocco",P::Midfielder,14,          83,6,7,55,35, 0,0, 0,68,36,22),
        makePlayer(3115,tid,"Bilal El Khannouss","Bilal","El Khannouss",21,"Morocco",P::Midfielder,20, 84,6,9,50,30, 0,0, 0,72,40,18),
        makePlayer(3116,tid,"Abdessamad Ezzalzouli","Abdessamad","Ezzalzouli",23,"Morocco",P::Midfielder,11, 80,5,8,45,25, 0,0, 0,75,35,15),

        // Forwards
        makePlayer(3117,tid,"Hakim Ziyech","Hakim","Ziyech",32,"Morocco",P::Forward,7,           84,12,14,12,5, 0,0,62,72,38,12),
        makePlayer(3118,tid,"Youssef En-Nesyri","Youssef","En-Nesyri",27,"Morocco",P::Forward,9, 68,22,6,10,4, 0,0,68,55,16,42),
        makePlayer(3119,tid,"Sofiane Boufal","Sofiane","Boufal",31,"Morocco",P::Forward,17,       76,10,10,10,4, 0,0,58,75,28,10),
        makePlayer(3120,tid,"Zakaria Aboukhlal","Zakaria","Aboukhlal",24,"Morocco",P::Forward,19, 70,10,7,10,4, 0,0,60,70,22,10),
        makePlayer(3121,tid,"Ibrahim Diaz","Ibrahim","Diaz",23,"Morocco",P::Forward,21,            68, 8,6, 8,3, 0,0,55,68,18, 8),
        makePlayer(3122,tid,"Walid Cheddira","Walid","Cheddira",26,"Morocco",P::Forward,11,        65,12,4, 8,3, 0,0,62,52,14,28),
    };
    return t;
}

// ── All teams ─────────────────────────────────────────────────────────────────

inline QVector<Models::Team> allTeams()
{
    return { brazil(), france(), england(), argentina(), germany(), spain(), morocco() };
}

} // namespace DemoData
