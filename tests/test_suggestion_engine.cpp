#include <QtTest/QtTest>
#include "engine/LineupSuggestionEngine.h"

// ── Test helper ───────────────────────────────────────────────────────────────

static Models::Player makePlayer(
    int id,
    const QString    &name,
    Models::Position  pos,
    // stats
    double passAcc    = 0,
    int    goals      = 0,
    int    assists    = 0,
    int    tackles    = 0,
    int    intercepts = 0,
    int    saves      = 0,
    int    cleanSheets = 0,
    double shotsOnPct = 0,
    double dribblesSuccPct = 0,
    int    keyPasses  = 0,
    int    aerialWon  = 0,
    double savePct    = 0)
{
    Models::Player p;
    p.id            = id;
    p.name          = name;
    p.position      = pos;
    p.jerseyNumber  = id % 23 + 1;
    p.teamId        = 1;

    Models::PlayerStats &s = p.stats;
    s.passAccuracy       = passAcc;
    s.goals              = goals;
    s.assists            = assists;
    s.tackles            = tackles;
    s.interceptions      = intercepts;
    s.saves              = saves;
    s.cleanSheets        = cleanSheets;
    s.shotsOnTargetPct   = shotsOnPct;
    s.dribbleSuccessPct  = dribblesSuccPct;
    s.keyPasses          = keyPasses;
    s.aerialDuelsWon     = aerialWon;
    s.savesPercentage    = savePct;
    return p;
}

// ── Test class ────────────────────────────────────────────────────────────────

class TestLineupSuggestionEngine : public QObject
{
    Q_OBJECT

private:
    LineupSuggestionEngine engine;

    QVector<Models::Player> buildFullSquad()
    {
        QVector<Models::Player> squad;

        // 2 GKs
        squad << makePlayer(1, "GK Alpha",   Models::Position::Goalkeeper,
                            80, 0, 0, 0, 0, 120, 8, 0, 0, 0, 0, 75.0);
        squad << makePlayer(2, "GK Beta",    Models::Position::Goalkeeper,
                            70, 0, 0, 0, 0,  60, 4, 0, 0, 0, 0, 60.0);

        // 6 DEFs
        squad << makePlayer(3, "DEF Aragorn", Models::Position::Defender,
                            82, 1, 2, 90, 50, 0, 0, 0, 0, 0, 40);
        squad << makePlayer(4, "DEF Boromir", Models::Position::Defender,
                            75, 0, 1, 70, 40, 0, 0, 0, 0, 0, 35);
        squad << makePlayer(5, "DEF Gimli",   Models::Position::Defender,
                            70, 0, 0, 60, 30, 0, 0, 0, 0, 0, 55);
        squad << makePlayer(6, "DEF Legolas", Models::Position::Defender,
                            85, 2, 3, 80, 60, 0, 0, 0, 0, 0, 45);
        squad << makePlayer(7, "DEF Sam",     Models::Position::Defender,
                            72, 0, 2, 55, 25, 0, 0, 0, 0, 0, 30);
        squad << makePlayer(8, "DEF Pippin",  Models::Position::Defender,
                            68, 0, 1, 50, 20, 0, 0, 0, 0, 0, 28);

        // 6 MIDs
        squad << makePlayer(9,  "MID Gandalf", Models::Position::Midfielder,
                            90, 4, 8, 30, 20, 0, 0, 0, 65, 40);
        squad << makePlayer(10, "MID Frodo",   Models::Position::Midfielder,
                            88, 3, 6, 25, 15, 0, 0, 0, 72, 35);
        squad << makePlayer(11, "MID Merry",   Models::Position::Midfielder,
                            82, 2, 4, 20, 10, 0, 0, 0, 58, 28);
        squad << makePlayer(12, "MID Bilbo",   Models::Position::Midfielder,
                            75, 1, 2, 15,  8, 0, 0, 0, 45, 20);
        squad << makePlayer(13, "MID Thorin",  Models::Position::Midfielder,
                            70, 0, 3, 18,  9, 0, 0, 0, 40, 18);
        squad << makePlayer(14, "MID Balin",   Models::Position::Midfielder,
                            65, 1, 1, 12,  6, 0, 0, 0, 30, 14);

        // 4 FWDs
        squad << makePlayer(15, "FWD Sauron",  Models::Position::Forward,
                            70, 28, 8, 0, 0, 0, 0, 68, 70, 0);
        squad << makePlayer(16, "FWD Saruman", Models::Position::Forward,
                            65, 20, 6, 0, 0, 0, 0, 60, 60, 0);
        squad << makePlayer(17, "FWD Gollum",  Models::Position::Forward,
                            55, 10, 4, 0, 0, 0, 0, 45, 55, 0);
        squad << makePlayer(18, "FWD Shelob",  Models::Position::Forward,
                            50,  5, 2, 0, 0, 0, 0, 35, 45, 0);

        return squad;
    }

private slots:

    // ── Scoring tests ─────────────────────────────────────────────────────────

    void testGkScoringPositive()
    {
        const auto gk = makePlayer(1, "GK", Models::Position::Goalkeeper,
                                   80, 0, 0, 0, 0, 100, 10, 0, 0, 0, 0, 80.0);
        const double score = engine.scorePlayer(gk, Models::Position::Goalkeeper);
        QVERIFY(score > 0.0);
    }

    void testGkScoringBetterThanWorse()
    {
        const auto gkGood = makePlayer(1, "GK Good", Models::Position::Goalkeeper,
                                       85, 0, 0, 0, 0, 150, 12, 0, 0, 0, 0, 85.0);
        const auto gkBad  = makePlayer(2, "GK Bad",  Models::Position::Goalkeeper,
                                       50, 0, 0, 0, 0,  20,  1, 0, 0, 0, 0, 40.0);
        QVERIFY(engine.scorePlayer(gkGood, Models::Position::Goalkeeper) >
                engine.scorePlayer(gkBad,  Models::Position::Goalkeeper));
    }

    void testDefScoringRewardsTackles()
    {
        const auto defGood = makePlayer(1, "DEF Tackle", Models::Position::Defender,
                                        80, 0, 0, 100, 60, 0, 0, 0, 0, 0, 50);
        const auto defBad  = makePlayer(2, "DEF Weak",   Models::Position::Defender,
                                        60, 0, 0,  10,  5, 0, 0, 0, 0, 0, 20);
        QVERIFY(engine.scorePlayer(defGood, Models::Position::Defender) >
                engine.scorePlayer(defBad,  Models::Position::Defender));
    }

    void testMidScoringRewardsPassAccuracy()
    {
        const auto midA = makePlayer(1, "MID High Pass", Models::Position::Midfielder,
                                     95, 5, 10, 0, 0, 0, 0, 0, 70, 50);
        const auto midB = makePlayer(2, "MID Low Pass",  Models::Position::Midfielder,
                                     40, 1,  2, 0, 0, 0, 0, 0, 30, 10);
        QVERIFY(engine.scorePlayer(midA, Models::Position::Midfielder) >
                engine.scorePlayer(midB, Models::Position::Midfielder));
    }

    void testFwdScoringRewardsGoals()
    {
        const auto fwdA = makePlayer(1, "FWD Prolific", Models::Position::Forward,
                                     70, 30, 5, 0, 0, 0, 0, 70, 65, 0);
        const auto fwdB = makePlayer(2, "FWD Barren",   Models::Position::Forward,
                                     60,  2, 1, 0, 0, 0, 0, 30, 40, 0);
        QVERIFY(engine.scorePlayer(fwdA, Models::Position::Forward) >
                engine.scorePlayer(fwdB, Models::Position::Forward));
    }

    // ── Suggestion tests ──────────────────────────────────────────────────────

    void testSuggestLineupReturns11()
    {
        const auto squad       = buildFullSquad();
        const auto suggestions = engine.suggestLineup(QStringLiteral("4-4-2"), squad);
        QCOMPARE(suggestions.size(), 11);
    }

    void testSuggestLineupNoPlayerDuplicated()
    {
        const auto squad       = buildFullSquad();
        const auto suggestions = engine.suggestLineup(QStringLiteral("4-3-3"), squad);

        QVector<int> ids;
        for (const auto &sp : suggestions) {
            QVERIFY2(!ids.contains(sp.player.id),
                     qPrintable(QString("Player %1 is duplicated!").arg(sp.player.name)));
            ids.append(sp.player.id);
        }
    }

    void testSuggestLineupFirstSlotIsGK()
    {
        const auto suggestions =
            engine.suggestLineup(QStringLiteral("4-4-2"), buildFullSquad());
        QVERIFY(!suggestions.isEmpty());
        QCOMPARE(suggestions.first().player.position, Models::Position::Goalkeeper);
    }

    void testSuggestLineup433Returns11()
    {
        const auto suggestions =
            engine.suggestLineup(QStringLiteral("4-3-3"), buildFullSquad());
        QCOMPARE(suggestions.size(), 11);
    }

    void testSuggestLineupBestGkSelected()
    {
        const auto suggestions =
            engine.suggestLineup(QStringLiteral("4-4-2"), buildFullSquad());
        // GK Alpha has higher stats — should be picked
        const Models::SuggestedPlayer &gkSlot = suggestions.first();
        QCOMPARE(gkSlot.player.id, 1);  // "GK Alpha"
    }

    // ── Compare formations tests ──────────────────────────────────────────────

    void testCompareFormationsReturnsAll()
    {
        const auto squad  = buildFullSquad();
        const auto scores = engine.compareFormations(squad);
        const int  expected = LineupSuggestionEngine::supportedFormations().size();
        QCOMPARE(scores.size(), expected);
    }

    void testCompareFormationsAllPositive()
    {
        const auto scores = engine.compareFormations(buildFullSquad());
        for (const double v : scores) {
            QVERIFY(v >= 0.0);
        }
    }

    // ── Edge case: empty squad ────────────────────────────────────────────────

    void testSuggestWithEmptySquad()
    {
        const auto suggestions =
            engine.suggestLineup(QStringLiteral("4-4-2"), {});
        // Should return 0 suggestions (no players available)
        QCOMPARE(suggestions.size(), 0);
    }

    // ── Formation parsing ─────────────────────────────────────────────────────

    void testSupportedFormationsNotEmpty()
    {
        QVERIFY(!LineupSuggestionEngine::supportedFormations().isEmpty());
    }

    void testSupportedFormationsTotalEleven()
    {
        for (const auto &fs : LineupSuggestionEngine::supportedFormations()) {
            const int total = fs.gk + fs.def + fs.mid + fs.fwd;
            QCOMPARE(total, 11);
        }
    }
};

QTEST_MAIN(TestLineupSuggestionEngine)
#include "test_suggestion_engine.moc"
