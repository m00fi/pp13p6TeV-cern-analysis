#include "Pythia8/Pythia.h"

#include <TFile.h>
#include <TTree.h>

#include <filesystem>
#include <iostream>
#include <vector>

namespace {

const char* const configPath = "config/config.cmnd";
const char* const outputDir = "data";
const char* const outputPath = "data/events.root";

// particles of a single event, one vector entry per particle.
struct EventData {
    std::vector<int> pdg;
    std::vector<int> charge;  // in units of e/3
    std::vector<float> pt;
    std::vector<float> eta;
    std::vector<float> phi;

    void connectTo(TTree& tree) {
        tree.Branch("pdg", &pdg);
        tree.Branch("charge", &charge);
        tree.Branch("pt", &pt);
        tree.Branch("eta", &eta);
        tree.Branch("phi", &phi);
    }

    void clear() {
        pdg.clear();
        charge.clear();
        pt.clear();
        eta.clear();
        phi.clear();
    }

    void add(const Pythia8::Particle& particle) {
        pdg.push_back(particle.id());
        charge.push_back(particle.chargeType());
        pt.push_back(particle.pT());
        eta.push_back(particle.eta());
        phi.push_back(particle.phi());
    }
};

}  // namespace

int main() {
    Pythia8::Pythia pythia;
    if (!pythia.readFile(configPath) || !pythia.init()) {
        std::cerr << "Pythia setup failed, config: " << configPath << '\n';
        return 1;
    }

    std::filesystem::create_directories(outputDir);
    TFile file(outputPath, "RECREATE");
    TTree tree("events", "Final-state particles");

    EventData event;
    event.connectTo(tree);

    const int nEvents = pythia.mode("Main:numberOfEvents");
    for (int iEvent = 0; iEvent < nEvents; ++iEvent) {
        if (!pythia.next()) continue;

        event.clear();
        for (int i = 0; i < pythia.event.size(); ++i) {
            const Pythia8::Particle& particle = pythia.event[i];
            if (particle.isFinal()) event.add(particle);
        }
        tree.Fill();
    }

    pythia.stat();
    tree.Write();
    file.Close();
    return 0;
}
