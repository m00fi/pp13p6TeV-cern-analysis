#include <TFile.h>
#include <TH1.h>
#include <TH1D.h>
#include <TMath.h>
#include <TTree.h>

#include <cmath>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

const char* const inputPath = "data/events.root";
const char* const outputDir = "results";
const char* const outputPath = "results/analysis.root";

// Particle selection and histograms of one category.
struct Category {
    bool (*matches)(int pdg, int charge);
    int count = 0;  // particles in the current event
    TH1D hN;
    TH1D hPt;
    TH1D hEta;
    TH1D hPhi;

    Category(const std::string& name, const std::string& label, bool (*selection)(int, int))
        : matches(selection),
          hN(("hN_" + name).c_str(), (label + ";Multiplicity N;Events").c_str(), 400, 0.0, 400.0),
          hPt(("hPt_" + name).c_str(), (label + ";p_{T} [GeV];Particles").c_str(), 100, 0.0, 10.0),
          hEta(("hEta_" + name).c_str(), (label + ";#eta;Particles").c_str(), 100, -5.0, 5.0),
          hPhi(("hPhi_" + name).c_str(), (label + ";#phi [rad];Particles").c_str(), 64, 0.0, TMath::Pi()) {}

    void write() {
        hN.Write();
        hPt.Write();
        hEta.Write();
        hPhi.Write();
    }
};

std::vector<Category> makeCategories() {
    std::vector<Category> categories;
    categories.emplace_back("all", "All final-state particles", [](int, int) { return true; });
    categories.emplace_back("ch", "Charged particles", [](int, int charge) { return charge != 0; });
    categories.emplace_back("pip", "#pi^{+}", [](int pdg, int) { return pdg == 211; });
    categories.emplace_back("pim", "#pi^{-}", [](int pdg, int) { return pdg == -211; });
    categories.emplace_back("Kp", "K^{+}", [](int pdg, int) { return pdg == 321; });
    categories.emplace_back("Km", "K^{-}", [](int pdg, int) { return pdg == -321; });
    categories.emplace_back("p", "p", [](int pdg, int) { return pdg == 2212; });
    categories.emplace_back("pbar", "#bar{p}", [](int pdg, int) { return pdg == -2212; });
    return categories;
}

}  // namespace

int main() {
    TH1::AddDirectory(false);

    TFile input(inputPath, "READ");
    if (input.IsZombie()) {
        std::cerr << "Cannot open input file: " << inputPath << '\n';
        return 1;
    }

    TTree* tree = input.Get<TTree>("events");
    if (!tree) {
        std::cerr << "Tree 'events' not found in: " << inputPath << '\n';
        return 1;
    }

    std::vector<int>* pdg = nullptr;
    std::vector<int>* charge = nullptr;
    std::vector<float>* pt = nullptr;
    std::vector<float>* eta = nullptr;
    std::vector<float>* phi = nullptr;
    tree->SetBranchAddress("pdg", &pdg);
    tree->SetBranchAddress("charge", &charge);
    tree->SetBranchAddress("pt", &pt);
    tree->SetBranchAddress("eta", &eta);
    tree->SetBranchAddress("phi", &phi);

    std::vector<Category> categories = makeCategories();

    const Long64_t nEvents = tree->GetEntries();
    for (Long64_t iEvent = 0; iEvent < nEvents; ++iEvent) {
        tree->GetEntry(iEvent);

        for (Category& category : categories) category.count = 0;

        for (std::size_t iParticle = 0; iParticle < pdg->size(); ++iParticle) {
            for (Category& category : categories) {
                if (!category.matches((*pdg)[iParticle], (*charge)[iParticle])) continue;

                ++category.count;
                category.hPt.Fill((*pt)[iParticle]);
                category.hEta.Fill((*eta)[iParticle]);
                // [0, pi]
                category.hPhi.Fill(std::abs((*phi)[iParticle]));
            }
        }

        // Filled for every event, also when N = 0.
        for (Category& category : categories) category.hN.Fill(category.count);
    }

    std::filesystem::create_directories(outputDir);
    TFile output(outputPath, "RECREATE");
    
    if (output.IsZombie()) {
        std::cerr << "Cannot create output file: " << outputPath << '\n';
        return 1;
    }

    for (Category& category : categories) category.write();
    output.Close();

    std::cout << "Processed " << nEvents << " events, histograms saved to " << outputPath << '\n';
    return 0;
}
