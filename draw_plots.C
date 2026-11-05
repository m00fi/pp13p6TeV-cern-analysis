#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TLegend.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>

#include <iostream>
#include <string>
#include <vector>

namespace {

const char* const inputPath = "results/analysis.root";
const char* const outputDir = "results/plots";
const char* const summaryPdfPath = "results/plots/all_plots.pdf";

struct Variable {
    std::string prefix;  // histogram name prefix
    std::string title;   // plot title before the category label
    bool logY;           // log scale
};

const std::vector<Variable> variables = {
    {"hN", "Multiplicity", true},
    {"hPt", "Transverse momentum", true},
    {"hEta", "Pseudorapidity", false},
    {"hPhi", "Azimuthal angle", false},
};

struct Category {
    std::string name;  // histogram name suffix
    Color_t color;
};

const std::vector<Category> categories = {
    {"all", kBlack},
    {"ch", kGray + 2},
    {"pip", kBlue + 1},
    {"pim", kAzure + 7},
    {"Kp", kGreen + 2},
    {"Km", kSpring - 6},
    {"p", kRed + 1},
    {"pbar", kOrange + 7},
};

void setStyle() {
    gStyle->SetOptStat(0);
    gStyle->SetTitleFontSize(0.05);
    gStyle->SetLabelSize(0.04, "XY");
    gStyle->SetTitleSize(0.045, "XY");
    gStyle->SetTitleOffset(1.2, "Y");
}

TLegend* makeLegend(TH1D* hist) {
    auto* legend = new TLegend(0.67, 0.75, 0.92, 0.92);
    legend->SetBorderSize(1);
    legend->SetFillColor(kWhite);
    legend->SetTextSize(0.035);
    legend->SetMargin(0.05);  // no marker column, text starts at the left edge
    legend->AddEntry(static_cast<TObject*>(nullptr), Form("Entries: %.0f", hist->GetEntries()), "");
    // Mean and standard deviation use only the entries inside the axis range.
    legend->AddEntry(static_cast<TObject*>(nullptr), Form("Mean: %.4g", hist->GetMean()), "");
    legend->AddEntry(static_cast<TObject*>(nullptr), Form("Std Dev: %.4g", hist->GetStdDev()), "");

    // Entries below or above the axis range are counted but not drawn
    const double outside = hist->GetBinContent(0) + hist->GetBinContent(hist->GetNbinsX() + 1);
    if (outside > 0) {
        legend->AddEntry(static_cast<TObject*>(nullptr), Form("Outside range: %.0f", outside), "");
    }
    return legend;
}

}  // namespace

void draw_plots() {
    TFile input(inputPath, "READ");
    if (input.IsZombie()) {
        std::cerr << "Cannot open input file: " << inputPath << '\n';
        return;
    }

    setStyle();
    gSystem->mkdir(outputDir, true);
    
    TCanvas summary("summary", "All plots", 800, 600);
    summary.Print(Form("%s[", summaryPdfPath));

    int nSaved = 0;
    for (const Variable& variable : variables) {
        for (const Category& category : categories) {
            const std::string name = variable.prefix + "_" + category.name;
            auto* hist = input.Get<TH1D>(name.c_str());
            if (!hist) {
                std::cerr << "Histogram not found: " << name << '\n';
                continue;
            }

            TCanvas canvas(("c_" + name).c_str(), name.c_str(), 800, 600);
            canvas.SetLeftMargin(0.13);
            canvas.SetBottomMargin(0.12);
            canvas.SetLogy(variable.logY);

            const std::string label = hist->GetTitle();
            hist->SetTitle((variable.title + ": " + label).c_str());
            hist->SetLineColor(category.color);
            hist->SetLineWidth(2);

            // Leave room above the data for the legend
            const double maximum = hist->GetMaximum();
            if (variable.logY) {
                hist->SetMinimum(0.5);
                hist->SetMaximum(maximum * 100.0);
            } else {
                hist->SetMinimum(0.0);
                hist->SetMaximum(maximum * 1.7);
            }
            hist->Draw("HIST");

            TLegend* legend = makeLegend(hist);
            legend->Draw();

            const std::string basePath = std::string(outputDir) + "/" + name;
            canvas.SaveAs((basePath + ".png").c_str());
            canvas.SaveAs((basePath + ".pdf").c_str());
            canvas.Print(summaryPdfPath, ("Title:" + name).c_str());
            delete legend;
            ++nSaved;
        }
    }

    summary.Print(Form("%s]", summaryPdfPath));
    std::cout << "Saved " << nSaved << " plots to " << outputDir << '\n';
}
