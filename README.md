# Rozkłady cząstek w zderzeniach pp przy √s = 13.6 TeV

Symulacja zderzeń proton-proton w Pythii 8 i analiza cząstek końcowych w ROOT.
Pipeline generuje 100 000 zdarzeń minimum bias, tworzy histogramy krotności, pT, η i φ
dla ośmiu kategorii cząstek i rysuje je jako wykresy PNG i PDF.

Autor: Michał Chrostowski

## Struktura repozytorium

```
├── config/config.cmnd     # ustawienia Pythii: energia, tryb, liczba zdarzeń, seed
├── src/generate.cc        # generator: Pythia -> data/events.root
├── src/analyze.cc         # analiza: data/events.root -> results/analysis.root
├── draw_plots.C           # makro ROOT: results/analysis.root -> results/plots/
├── CMakeLists.txt         # budowanie programów generate i analyze
├── results/
│   ├── analysis.root      # 32 histogramy
│   ├── generate.log       # log Pythii: ustawienia, przekroje czynne, błędy i ostrzeżenia
│   ├── analysis.log       # liczby cząstek i statystyki każdego histogramu
│   └── plots/             # wykresy PNG i PDF oraz zbiorczy all_plots.pdf
└── data/events.root       # cząstki końcowe ze wszystkich zdarzeń (poza repo, ok. 225 MB)
```

## Wymagania

- Linux (sprawdzone na Ubuntu 24.04)
- [Miniforge](https://github.com/conda-forge/miniforge), czyli conda z kanałem `conda-forge`
- ROOT 6.40.04, Pythia 8.312, CMake i kompilator C++ z `conda-forge`

## Instalacja środowiska

```bash
curl -L -O https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-x86_64.sh
bash Miniforge3-Linux-x86_64.sh
```

W nowym terminalu:

```bash
conda create -n pp13p6 -c conda-forge root=6.40.04 pythia8=8.312 cmake make compilers
conda activate pp13p6
```

Wszystkie poniższe komendy uruchamiamy z głównego katalogu repozytorium, przy aktywnym środowisku `pp13p6`.

## Kompilacja

```bash
cmake -S . -B build
cmake --build build -j8
```

Powstają dwa programy: `build/generate` i `build/analyze`.

## Uruchomienie

```bash
mkdir -p results
./build/generate > results/generate.log 2>&1   # generacja zdarzeń
./build/analyze | tee results/analysis.log     # histogramy
root -l -b -q draw_plots.C                     # wykresy
```

| Etap | Wejście | Wyjście |
|---|---|---|
| `generate` | `config/config.cmnd` | `data/events.root`: drzewo `events`, jeden wpis na zdarzenie |
| `analyze` | `data/events.root` | `results/analysis.root`: 32 histogramy |
| `draw_plots.C` | `results/analysis.root` | `results/plots/`: 32 × PNG, 32 × PDF i `all_plots.pdf` |

Generacja 100 000 zdarzeń trwała 3 min 33 s na jednym rdzeniu laptopa, a analiza około 6 s.

Każdy przebieg zostawia logi w `results/`, więc liczby z tego README można sprawdzić u źródła:

- [generate.log](results/generate.log): log Pythii, w tym pełny zapis pierwszego zdarzenia,
  przekroje czynne i statystyka błędów i ostrzeżeń
- [analysis.log](results/analysis.log): liczba cząstek w każdej kategorii, skład cząstek
  naładowanych oraz liczba wpisów, średnia, odchylenie standardowe, underflow i overflow
  każdego histogramu

Drzewo `events` ma dla każdej cząstki końcowej gałęzie `pdg`, `charge` (w jednostkach e/3),
`pt` [GeV], `eta` i `phi` [rad].

## Odtwarzalność

Ustawienia generatora są w [config/config.cmnd](config/config.cmnd):

```
Main:numberOfEvents = 100000
Beams:eCM = 13600.
SoftQCD:inelastic = on
Random:setSeed = on
Random:seed = 123
```

Przy tych samych wersjach ROOT-a i Pythii ten sam seed daje identyczne histogramy.
Pliki `.root` nie są identyczne bajt w bajt, bo zawierają znaczniki czasu,
więc porównujemy zawartość histogramów, a nie same pliki.

## Założenia

**Generacja**

- `SoftQCD:inelastic = on`, czyli minimum bias bez zderzeń elastycznych
- domyślny tune Pythii, Monash 2013 (`Tune:pp = 14`)
- analizowane są tylko cząstki końcowe (`isFinal()`)
- domyślne ustawienia rozpadów Pythii: π± i K± są stabilne, a K⁰ₛ i Λ się rozpadają,
  więc część pionów i protonów pochodzi z tych rozpadów

**Kategorie cząstek**

| Sufiks | Cząstki | Warunek |
|---|---|---|
| `all` | wszystkie cząstki końcowe, także neutralne | brak |
| `ch` | cząstki naładowane | ładunek ≠ 0 |
| `pip`, `pim` | π⁺, π⁻ | PDG +211, −211 |
| `Kp`, `Km` | K⁺, K⁻ | PDG +321, −321 |
| `p`, `pbar` | p, p̄ | PDG +2212, −2212 |

**Histogramy**

Każda kategoria ma histogramy `hN_*`, `hPt_*`, `hEta_*` i `hPhi_*`, razem 32.

| Histogram | Zakres | Liczba binów | Jeden wpis to |
|---|---|---|---|
| `hN_*` | 0–400 | 400 | zdarzenie |
| `hPt_*` | 0–10 GeV | 100 | cząstka |
| `hEta_*` | −5 do 5 | 100 | cząstka |
| `hPhi_*` | −π do π | 64 | cząstka |

- `hN_*` wypełniamy dla każdego zdarzenia, także gdy N = 0.
- Polecenie podaje dla φ zakres 0–π, ale Pythia zwraca φ w przedziale (−π, π]. Żeby histogram
  obejmował wszystkie cząstki, `hPhi_*` ma pełny przedział od −π do π, przy zachowaniu 64 binów
  z polecenia.
- Wartości spoza zakresu histogramu trafiają do binów underflow i overflow, a średnia
  i odchylenie standardowe uwzględniają je (`TH1::StatOverflows(true)`).

## Wyniki

| Kategoria | Cząstek na zdarzenie | Średnie pT [GeV] | Poza \|η\| < 5 |
|---|---|---|---|
| `all` | 189.7 | 0.40 | 25.8% |
| `ch` | 90.7 | 0.51 | 26.9% |
| `pip` / `pim` | 37.8 / 37.1 | 0.48 / 0.48 | 26.2% / 25.1% |
| `Kp` / `Km` | 4.15 / 4.04 | 0.68 / 0.68 | 27.7% / 26.0% |
| `p` / `pbar` | 3.82 / 2.60 | 0.69 / 0.78 | 50.5% / 28.9% |

- **Przebieg:** Pythia wygenerowała wszystkie 100 000 zdarzeń, w tym 72.3% niedyfrakcyjnych
  i 27.7% dyfrakcyjnych. Szacowany przekrój czynny na zderzenia nieelastyczne to 78.6 mb.
- **Skład:** wśród cząstek naładowanych 82.6% to piony, 9.0% kaony, 7.1% protony i antyprotony,
  a 1.3% inne. Na zdarzenie przypada też średnio 88.9 fotonów, głównie z rozpadów π⁰ → γγ,
  stąd `all` ≈ 2 × `ch`.
- **Protony:** jest ich więcej niż antyprotonów, bo zderzają się dwa protony. Połowa z nich ma
  |η| > 5, bo część to protony z wiązki, które przetrwały zderzenia dyfrakcyjne.
- **Kontrola poprawności:** φ jest płaskie (odchylenie 1.814 ≈ π/√3), η symetryczne wokół 0,
  a N<sub>ch</sub> zawsze parzyste, bo protony na wejściu mają łączny ładunek +2.

## Uwagi do interpretacji wykresów

- **Zakresy z polecenia obcinają część rozkładów:** 12.3% zdarzeń ma N<sub>all</sub> ≥ 400,
  65 zdarzeń ma N<sub>ch</sub> ≥ 400, a około 27% cząstek ma |η| > 5. Te wpisy trafiają do underflow lub overflow: legenda podaje ich liczbę
  w wierszu `Outside range`, a `Mean` i `Std Dev` je uwzględniają.
- **Krotności rysujemy punktami:** co drugi bin `hN_ch` jest pusty, bo N<sub>ch</sub> jest
  parzyste, więc linia spadałaby w nich do zera. 400 binów zostaje, bo tego wymaga polecenie.

## Źródła

- C. Bierlich i in., *A comprehensive guide to the physics and usage of PYTHIA 8.3*,
  SciPost Phys. Codebases 8 (2022), [arXiv:2203.11601](https://arxiv.org/abs/2203.11601)
- Dokumentacja Pythii 8: [pythia.org](https://pythia.org/), m.in. ustawienia `SoftQCD`,
  ziarno generatora i właściwości cząstek
- P. Skands, S. Carrazza, J. Rojo, *Tuning PYTHIA 8.1: the Monash 2013 Tune*,
  Eur. Phys. J. C 74 (2014) 3024, [arXiv:1404.5630](https://arxiv.org/abs/1404.5630)
- Dokumentacja ROOT: [root.cern](https://root.cern/), m.in. [ROOT Primer](https://root.cern/primer/)
  i [podręcznik](https://root.cern/manual/) (histogramy, `TTree`, `TCanvas`)
- Particle Data Group, [Review of Particle Physics](https://pdg.lbl.gov/): numeracja cząstek
  Monte Carlo (kody PDG) oraz przegląd *Kinematics* (pT, η, φ)

## Licencja

Projekt jest udostępniony na licencji [MIT](LICENSE).
