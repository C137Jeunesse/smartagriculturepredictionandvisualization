#ifndef PREDICTION_H
#define PREDICTION_H

#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <iosfwd>
#include <QDebug>
#include <QFileInfo>

namespace FertilizerPrediction {

// Data Structures
struct FertilizerDataPoint {
    double temperature, humidity, moisture, nitrogen, potassium, phosphorous;
    std::string soilType, cropType, fertilizer;
};

struct PredictionResult {
    std::string fertilizer;
    double confidence;
};

// Class Declarations
class FeatureEncoder;
class DecisionTreeClassifier;
class RandomForestClassifier;
class FertilizerPredictionSystem;

// Full Class Definitions (Declarations Only)
class FeatureEncoder {
public:
    FeatureEncoder();
    int encodeSoilType(const std::string& type);
    int encodeCropType(const std::string& type);
    int encodeFertilizer(const std::string& type);
    std::string decodeFertilizer(int code) const;
    void serialize(std::ostream& os) const;
    void deserialize(std::istream& is);
private:
    friend class DecisionTreeClassifier;
    std::unordered_map<std::string, int> soilTypeMap, cropTypeMap, fertilizerMap;
    int nextSoilTypeCode, nextCropTypeCode, nextFertilizerCode;
};

class DecisionTreeClassifier {
public:
    explicit DecisionTreeClassifier(FeatureEncoder* encoder);
    ~DecisionTreeClassifier(); // Add destructor declaration
    DecisionTreeClassifier(DecisionTreeClassifier&& other) noexcept; // Add move constructor declaration
    DecisionTreeClassifier& operator=(DecisionTreeClassifier&& other) noexcept; // Add move assignment declaration

    void train(const std::vector<FertilizerDataPoint*>& data);
    std::string predict(const FertilizerDataPoint& point) const;
    void serialize(std::ostream& os) const;
    void deserialize(std::istream& is);
private:
    struct DecisionTreeNode;
    FeatureEncoder* encoder;
    std::unique_ptr<DecisionTreeNode> root;
    int getFeatureValue(const FertilizerDataPoint& point, int featureIndex) const;
    std::string predictRecursive(const DecisionTreeNode* node, const FertilizerDataPoint& point) const;
};

class RandomForestClassifier {
public:
    explicit RandomForestClassifier(FeatureEncoder* encoder, int n_trees = 50);
    void train(std::vector<FertilizerDataPoint>& data);
    std::pair<std::vector<PredictionResult>, std::string> predictWithConfidence(const std::string& input);
    void serialize(std::ostream& os) const;
    void deserialize(std::istream& is);
private:
    std::vector<std::unique_ptr<DecisionTreeClassifier>> trees;
    FeatureEncoder* encoder;
    int numTrees;
};

class FertilizerPredictionSystem {
public:
    FertilizerPredictionSystem();
    ~FertilizerPredictionSystem();
    bool loadDataFromCSV(const std::string& filename);
    bool trainModel();
    bool isModelReady() const;
    std::pair<std::vector<PredictionResult>, std::string> predict(const std::string& input);
    bool saveModel(const std::string& filename);
    bool loadModel(const std::string& filename);
private:
    FeatureEncoder encoder;
    std::unique_ptr<RandomForestClassifier> model;
    std::vector<FertilizerDataPoint> trainingData;
    bool isModelTrained;
};

FertilizerDataPoint parseInputString(const std::string& input);

} // namespace
#endif // PREDICTION_H
