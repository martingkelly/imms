#include "cluster.h"
#include "utils.h"

AcousticCluster::AcousticCluster() : ad(get_imms_root("cluster.db"), "Cluster")
{
    create_sql_tables();
}

void AcousticCluster::create_sql_tables()
{
    Q("CREATE TABLE Cluster.Distances ("
            "'x' INTEGER NOT NULL, "
            "'y' INTEGER NOT NULL, "
            "'distance' INTEGER NOT NULL);").execute();

    Q("CREATE UNIQUE INDEX Cluster.Distances_x_y_i "
            "ON Cluster.Distances (x, y);").execute();

    Q("CREATE INDEX Cluster.Distances_x_i "
            "ON 'Cluster.Distances' (x);").execute();
    Q("CREATE INDEX Cluster.Distances_y_i "
            "ON Cluster.Distances (y);").execute();
}

void AcousticCluster::calculate_distances(int from)
{
    if (from < 0)
    {
        from = 0;
        Q q("SELECT max(x) + 1 FROM Distances;");
        if (q.next())
            q >> from;
    }

    Q q("SELECT uid, spectrum, bpm FROM Acoustic "
            "WHERE uid >= ? ORDER BY uid ASC;");

    if (!q.next())
        return;

    int next;
    bool first = true;
    string fromspec, frombpm;

    q >> from >> fromspec >> frombpm;

    while (q.next())
    {
        int cur;
        string curspec, curbpm; 
        
        q >> cur >> curspec >> curbpm;

        if (first)
            next = cur;
        first = false;
    }

    if (!first)
        calculate_distances(next);

}
