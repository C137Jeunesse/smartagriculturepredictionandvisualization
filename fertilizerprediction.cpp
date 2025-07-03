#include "prediction.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include <random>
#include <numeric>
#include <sstream>
#include <fstream>
#include <iostream>
#include <functional>

namespace FertilizerPrediction {

// =================== FeatureEncoder Implementation ===================
FeatureEncoder::FeatureEncoder() : nextSoilTypeCode(0), nextCropTypeCode(0), nextFertilizerCode(0) {}

int FeatureEncoder::encodeSoilType(const std::string& soilType) {
    if (soilTypeMap.find(soilType) == soilTypeMap.end()) soilTypeMap[soilType] = nextSoilTypeCode++;
    return soilTypeMap.at(soilType);
}
int FeatureEncoder::encodeCropType(const std::string& cropType) {
    if (cropTypeMap.find(cropType) == cropTypeMap.end()) cropTypeMap[cropType] = nextCropTypeCode++;
    return cropTypeMap.at(cropType);
}
int FeatureEncoder::encodeFertilizer(const std::string& fertilizer) {
    if (fertilizerMap.find(fertilizer) == fertilizerMap.end()) fertilizerMap[fertilizer] = nextFertilizerCode++;
    return fertilizerMap.at(fertilizer);
}

// =================== Decision Tree Node & Classifier ===================
struct DecisionTreeClassifier::DecisionTreeNode {
    int featureIndex = -1;
    int splitValue = -1;
    std::string classLabel;
    bool isLeaf = false;
    std::shared_ptr<DecisionTreeNode> left;
    std::shared_ptr<DecisionTreeNode> right;
};

namespace { // Anonymous namespace for internal helper functions

int discretize(double value, const std::vector<std::pair<double, double>>& ranges) {
    for (size_t i = 0; i < ranges.size(); ++i) {
        if (value >= ranges[i].first && value < ranges[i].second) return static_cast<int>(i);
    }
    return static_cast<int>(ranges.size()); // Return one past the last bin for values >= last max
}

int getFeatureValue(const FertilizerDataPoint& point, int featureIndex, FeatureEncoder& encoder) {
    static const std::vector<std::pair<double, double>> tempRanges = {{0,25},{25,30},{30,35}};
    static const std::vector<std::pair<double, double>> humRanges = {{0,50},{50,65},{65,80}};
    static const std::vector<std::pair<double, double>> moistRanges = {{0,30},{30,50},{50,70}};
    static const std::vector<std::pair<double, double>> nutrientRanges = {{0,15},{15,30},{30,45}};
    switch (featureIndex) {
    case 0: return discretize(point.temperature, tempRanges);
    case 1: return discretize(point.humidity, humRanges);
    case 2: return discretize(point.moisture, moistRanges);
    case 3: return encoder.encodeSoilType(point.soilType);
    case 4: return encoder.encodeCropType(point.cropType);
    case 5: return discretize(point.nitrogen, nutrientRanges);
    case 6: return discretize(point.potassium, nutrientRanges);
    case 7: return discretize(point.phosphorous, nutrientRanges);
    default: return 0;
    }
}

double calculateGini(const std::vector<const FertilizerDataPoint*>& data) {
    if (data.empty()) return 0.0;
    std::unordered_map<std::string, int> counts;
    for (const auto* p : data) counts[p->fertilizer]++;
    double gini = 1.0;
    for (const auto& pair : counts) {
        double prob = static_cast<double>(pair.second) / data.size();
        gini -= prob * prob;
    }
    return gini;
}

std::string getMajorityClass(const std::vector<const FertilizerDataPoint*>& data) {
    if (data.empty()) return "Unknown";
    std::unordered_map<std::string, int> counts;
    for (const auto* p : data) counts[p->fertilizer]++;
    return std::max_element(counts.begin(), counts.end(),
                            [](const auto& a, const auto& b) { return a.second < b.second; })->first;
}

} // end anonymous namespace

DecisionTreeClassifier::DecisionTreeClassifier(FeatureEncoder& enc) : encoder(enc) {}

void DecisionTreeClassifier::train(const std::vector<FertilizerDataPoint>& data) {
    std::vector<const FertilizerDataPoint*> data_ptrs;
    data_ptrs.reserve(data.size());
    for(const auto& d : data) data_ptrs.push_back(&d);

    std::function<std::shared_ptr<DecisionTreeNode>(std::vector<const FertilizerDataPoint*>, int)> build_tree_recursive =
        [&](std::vector<const FertilizerDataPoint*> current_data, int depth) -> std::shared_ptr<DecisionTreeNode> {
        auto node = std::make_shared<DecisionTreeNode>();

        const int numFeatures = 8;
        const int minSamplesSplit = 5;
        const int maxDepth = 10;

        std::string majorityClass = getMajorityClass(current_data);
        if (depth >= maxDepth || current_data.size() < minSamplesSplit || calculateGini(current_data) < 1e-4) {
            node->isLeaf = true;
            node->classLabel = majorityClass;
            return node;
        }

        double bestGain = -1.0;
        int bestFeature = -1;
        int bestValue = -1;

        std::vector<int> feature_indices(numFeatures);
        std::iota(feature_indices.begin(), feature_indices.end(), 0);
        static std::mt19937 rng(std::random_device{}());
        std::shuffle(feature_indices.begin(), feature_indices.end(), rng);

        int featuresToConsider = static_cast<int>(sqrt(numFeatures)) + 1;
        double parentGini = calculateGini(current_data);

        for (int i = 0; i < featuresToConsider; ++i) {
            int feature = feature_indices[i];
            std::unordered_set<int> unique_values;
            for(const auto* p : current_data) unique_values.insert(getFeatureValue(*p, feature, encoder));

            for (int val : unique_values) {
                std::vector<const FertilizerDataPoint*> left, right;
                for (const auto* p : current_data) {
                    if (getFeatureValue(*p, feature, encoder) == val) left.push_back(p);
                    else right.push_back(p);
                }
                if (left.empty() || right.empty()) continue;

                double weightedGini = (static_cast<double>(left.size())/current_data.size() * calculateGini(left)) +
                                      (static_cast<double>(right.size())/current_data.size() * calculateGini(right));
                double gain = parentGini - weightedGini;
                if (gain > bestGain) {
                    bestGain = gain;
                    bestFeature = feature;
                    bestValue = val;
                }
            }
        }

        if (bestGain <= 0) {
            node->isLeaf = true;
            node->classLabel = majorityClass;
            return node;
        }

        node->featureIndex = bestFeature;
        node->splitValue = bestValue;

        std::vector<const FertilizerDataPoint*> left_data, right_data;
        for (const auto* p : current_data) {
            if (getFeatureValue(*p, node->featureIndex, encoder) == node->splitValue) left_data.push_back(p);
            else right_data.push_back(p);
        }

        if (left_data.empty() || right_data.empty()) {
            node->isLeaf = true;
            node->classLabel = majorityClass;
            return node;
        }

        node->left = build_tree_recursive(left_data, depth + 1);
        node->right = build_tree_recursive(right_data, depth + 1);
        return node;
    };

    if(!data.empty()) root = build_tree_recursive(data_ptrs, 0);
}

std::string DecisionTreeClassifier::predict(double t, double h, double m, const std::string& s, const std::string& c, double n, double k, double p) {
    if (!root) return "Unknown";
    FertilizerDataPoint point{t, h, m, s, c, n, k, p, ""};
    auto current_node = root;
    while (current_node && !current_node->isLeaf) {
        int val = getFeatureValue(point, current_node->featureIndex, encoder);
        if (val == current_node->splitValue) {
            current_node = current_node->left;
        } else {
            current_node = current_node->right;
        }
    }
    return current_node ? current_node->classLabel : "Unknown";
}

void DecisionTreeClassifier::saveModel(std::ostream& out) const {
    std::function<void(const std::shared_ptr<DecisionTreeNode>&)> serialize =
        [&](const std::shared_ptr<DecisionTreeNode>& node) {
            if (!node) {
                out << "#\n";
                return;
            }
            out << node->isLeaf << " " << node->featureIndex << " " << node->splitValue << " " << node->classLabel << "\n";
            if (!node->isLeaf) {
                serialize(node->left);
                serialize(node->right);
            }
        };
    serialize(root);
}

void DecisionTreeClassifier::loadModel(std::istream& in) {
    std::function<std::shared_ptr<DecisionTreeNode>()> deserialize =
        [&]() -> std::shared_ptr<DecisionTreeNode> {
        std::string line;
        if (!std::getline(in, line) || line == "#") return nullptr;

        std::stringstream ss(line);
        auto node = std::make_shared<DecisionTreeNode>();
        ss >> node->isLeaf >> node->featureIndex >> node->splitValue >> node->classLabel;

        if (!node->isLeaf) {
            node->left = deserialize();
            node->right = deserialize();
        }
        return node;
    };
    root = deserialize();
}

// =================== Random Forest Classifier ===================
RandomForestClassifier::RandomForestClassifier(FeatureEncoder& enc) : encoder(enc) {}

void RandomForestClassifier::train(const std::vector<FertilizerDataPoint>& data) {
    if (data.empty()) return;
    const int numTrees = 50;
    const double sampleRatio = 0.7;
    trees.clear();
    trees.reserve(numTrees);

    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, data.size() - 1);

    for (int i = 0; i < numTrees; ++i) {
        std::vector<FertilizerDataPoint> bootstrap_sample;
        size_t sample_size = static_cast<size_t>(data.size() * sampleRatio);
        bootstrap_sample.reserve(sample_size);
        for (size_t j = 0; j < sample_size; ++j) {
            bootstrap_sample.push_back(data[dist(rng)]);
        }
        trees.emplace_back(encoder);
        trees.back().train(bootstrap_sample);
    }
}

std::pair<std::vector<PredictionResult>, std::string> RandomForestClassifier::predictWithConfidence(const std::string& input) {
    if (trees.empty()) return {};
    std::stringstream ss(input);
    FertilizerDataPoint point;
    ss >> point.temperature >> point.humidity >> point.moisture >> point.soilType >> point.cropType
        >> point.nitrogen >> point.potassium >> point.phosphorous;

    std::unordered_map<std::string, int> votes;
    for (auto& tree : trees) {
        votes[tree.predict(point.temperature, point.humidity, point.moisture,
                           point.soilType, point.cropType, point.nitrogen,
                           point.potassium, point.phosphorous)]++;
    }

    std::vector<PredictionResult> results;
    for(const auto& v : votes) results.push_back({v.first, static_cast<double>(v.second) / trees.size()});
    std::sort(results.begin(), results.end(), [](const auto& a, const auto& b){ return a.confidence > b.confidence; });
    if (results.size() > 3) results.resize(3);

    std::string tip = (point.moisture < 40) ? "土壤湿度较低，建议及时浇水！" : "土壤湿度适中，暂无需浇水。";
    return {results, tip};
}

void RandomForestClassifier::saveModel(std::ostream& out) const {
    out << trees.size() << "\n";
    for(const auto& tree : trees) {
        tree.saveModel(out);
    }
}

void RandomForestClassifier::loadModel(std::istream& in) {
    size_t num_trees = 0;
    in >> num_trees;
    std::string dummy;
    std::getline(in, dummy); // consume rest of line
    trees.clear();
    trees.reserve(num_trees);
    for (size_t i = 0; i < num_trees; ++i) {
        trees.emplace_back(encoder);
        trees.back().loadModel(in);
    }
}


// =================== Fertilizer Prediction System ===================
FertilizerPredictionSystem::FertilizerPredictionSystem()
    : model(std::make_unique<RandomForestClassifier>(encoder)), isModelTrained(false) {}

FertilizerPredictionSystem::~FertilizerPredictionSystem() = default;

bool FertilizerPredictionSystem::loadDataFromCSV(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Cannot open CSV file: " << filename << std::endl;
        return false;
    }
    trainingData.clear();
    std::string line;
    std::getline(file, line); // Skip header
    while(std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;
        while(std::getline(ss, token, ',')) tokens.push_back(token);
        if (tokens.size() >= 10) {
            try {
                FertilizerDataPoint p;
                p.temperature = std::stod(tokens[1]);
                p.humidity = std::stod(tokens[2]);
                p.moisture = std::stod(tokens[3]);
                p.soilType = tokens[4];
                p.cropType = tokens[5];
                p.nitrogen = std::stod(tokens[6]);
                p.potassium = std::stod(tokens[7]);
                p.phosphorous = std::stod(tokens[8]);
                p.fertilizer = tokens[9];
                trainingData.push_back(p);
                encoder.encodeSoilType(p.soilType);
                encoder.encodeCropType(p.cropType);
                encoder.encodeFertilizer(p.fertilizer);
            } catch(const std::exception& e) {
                // std::cerr << "Skipping bad line: " << line << " | Error: " << e.what() << std::endl;
                continue;
            }
        }
    }
    std::cout << "Successfully loaded " << trainingData.size() << " records from " << filename << std::endl;
    return !trainingData.empty();
}

bool FertilizerPredictionSystem::trainModel() {
    if (trainingData.empty()) return false;
    std::cout << "Starting model training with " << trainingData.size() << " records..." << std::endl;
    model->train(trainingData);
    isModelTrained = true;
    std::cout << "Model training finished." << std::endl;
    return true;
}

bool FertilizerPredictionSystem::isModelReady() const { return isModelTrained; }

std::pair<std::vector<PredictionResult>, std::string> FertilizerPredictionSystem::predict(const std::string& input) {
    if (!isModelTrained) return {};
    return model->predictWithConfidence(input);
}

bool FertilizerPredictionSystem::saveModel(const std::string& filename) {
    if (!isModelTrained) return false;
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    model->saveModel(file);
    return true;
}

bool FertilizerPredictionSystem::loadModel(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    model->loadModel(file);
    // After loading, we must also populate the encoder with the correct mappings.
    // This is a simplification; a more robust solution saves/loads the encoder too.
    // For now, we rely on having trained or loaded data once.
    isModelTrained = true;
    return true;
}

} // namespace FertilizerPrediction
