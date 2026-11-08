#include <TFile.h>
#include <TH1.h>
#include <TH1D.h>
#include <TMath.h>
#include <TTree.h>

#include <cstddef>
#include <cstdio>
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
    std::string name;
    bool (*matches)(int pdg, int charge);
    int count = 0;  // particles in the current event
    TH1D hN;
    TH1D hPt;
    TH1D hEta;
    TH1D hPhi;

    Category(const std::string& name, const std::string& label, bool (*selection)(int, int))
        : name(name),
          matches(selection),
          hN(("hN_" + name).c_str(), (label + ";Multiplicity N;Events").c_str(), 400, 0.0, 400.0),
          hPt(("hPt_" + name).c_str(), (label + ";p_{T} [GeV];Particles").c_str(), 100, 0.0, 10.0),
          hEta(("hEta_" + name).c_str(), (label + ";#eta;Particles").c_str(), 100, -5.0, 5.0),
          hPhi(("hPhi_" + name).c_str(), (label + ";#phi [rad];Particles").c_str(), 64, -TMath::Pi(), TMath::Pi()) {}

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

double particlesIn(const std::vector<Category>& categories, const std::string& name) {
    for (const Category& category : categories) {
        if (category.name == name) return category.hPt.GetEntries();
    }
    return 0.0;
}

void printHistogramStats(const TH1D& hist) {
    const int nBins = hist.GetNbinsX();
    std::printf("  %-10s %10.0f %12.5g %12.5g %10.0f %10.0f\n", hist.GetName(), hist.GetEntries(), hist.GetMean(),
                hist.GetStdDev(), hist.GetBinContent(0), hist.GetBinContent(nBins + 1));
}

// Writes the numbers quoted in the README; save them with: ./build/analyze | tee results/analysis.log
void printSummary(const std::vector<Category>& categories, Long64_t nEvents, Long64_t nPhotons, Long64_t nOddCharged) {
    const double events = static_cast<double>(nEvents);
    std::printf("Events: %lld\n\n", nEvents);

    std::printf("Particles per category\n");
    std::printf("  %-6s %12s %10s\n", "name", "particles", "per event");
    for (const Category& category : categories) {
        const double particles = category.hPt.GetEntries();
        std::printf("  %-6s %12.0f %10.3f\n", category.name.c_str(), particles, particles / events);
    }

    const double charged = particlesIn(categories, "ch");
    const double pions = particlesIn(categories, "pip") + particlesIn(categories, "pim");
    const double kaons = particlesIn(categories, "Kp") + particlesIn(categories, "Km");
    const double protons = particlesIn(categories, "p") + particlesIn(categories, "pbar");
    std::printf("\nComposition of charged particles\n");
    std::printf("  pi+-       %5.1f%%\n", 100.0 * pions / charged);
    std::printf("  K+-        %5.1f%%\n", 100.0 * kaons / charged);
    std::printf("  p, pbar    %5.1f%%\n", 100.0 * protons / charged);
    std::printf("  other      %5.1f%%\n", 100.0 * (charged - pions - kaons - protons) / charged);

    std::printf("\nPhotons per event: %.2f\n", nPhotons / events);
    std::printf("Events with an odd number of charged particles: %lld\n", nOddCharged);

    std::printf("\nHistogram statistics (mean and std dev include underflow and overflow)\n");
    std::printf("  %-10s %10s %12s %12s %10s %10s\n", "histogram", "entries", "mean", "std dev", "underflow",
                "overflow");
    for (const Category& category : categories) {
        printHistogramStats(category.hN);
        printHistogramStats(category.hPt);
        printHistogramStats(category.hEta);
        printHistogramStats(category.hPhi);
    }
    std::printf("\n");
}

}  // namespace

int main() {
    TH1::AddDirectory(false);
    // Mean and standard deviation include entries outside the axis range.
    TH1::StatOverflows(true);

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

    Long64_t nPhotons = 0;
    Long64_t nOddCharged = 0;  // events with an odd number of charged particles

    const Long64_t nEvents = tree->GetEntries();
    for (Long64_t iEvent = 0; iEvent < nEvents; ++iEvent) {
        tree->GetEntry(iEvent);

        for (Category& category : categories) category.count = 0;
        int nCharged = 0;

        for (std::size_t iParticle = 0; iParticle < pdg->size(); ++iParticle) {
            if ((*pdg)[iParticle] == 22) ++nPhotons;
            if ((*charge)[iParticle] != 0) ++nCharged;

            for (Category& category : categories) {
                if (!category.matches((*pdg)[iParticle], (*charge)[iParticle])) continue;

                ++category.count;
                category.hPt.Fill((*pt)[iParticle]);
                category.hEta.Fill((*eta)[iParticle]);
                category.hPhi.Fill((*phi)[iParticle]);
            }
        }

        // Filled for every event, also when N = 0.
        for (Category& category : categories) category.hN.Fill(category.count);
        if (nCharged % 2 != 0) ++nOddCharged;
    }

    printSummary(categories, nEvents, nPhotons, nOddCharged);

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
