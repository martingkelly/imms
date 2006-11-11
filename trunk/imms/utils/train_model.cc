const char *help = "\
SVMTorch III (c) Trebolloc & Co 2002\n\
\n\
This program will train a SVM for classification or regression\n\
with a gaussian kernel (default) or a polynomial kernel.\n\
It supposes that the file you provide contains two classes,\n\
except if you use the '-class' option which trains one class\n\
against the others.\n";

#include <torch/MatDataSet.h>
#include <torch/TwoClassFormat.h>
#include <torch/ClassMeasurer.h>
#include <torch/MSEMeasurer.h>
#include <torch/QCTrainer.h>
#include <torch/CmdLine.h>
#include <torch/Random.h>
#include <torch/MeanVarNorm.h>
#include <torch/SVMClassification.h>
#include <torch/KFold.h>
#include <torch/DiskXFile.h>
#include <torch/ClassFormatDataSet.h>

#include <assert.h>
#include <iostream>

#include <model/model.h>
#include <immscore/immsutil.h>

const string AppName = "train_model";

using namespace Torch;
using namespace std;

int main(int argc, char **argv)
{
  char *file;
  real c_cst, stdv = 12;
  real accuracy, cache_size;
  int iter_shrink, k_fold;
  char *dir_name;
  char *model_file;
  bool binary_mode;
  real a_cst, b_cst;

  Allocator *allocator = new Allocator;

  //=================== The command-line ==========================

  // Construct the command line
  CmdLine cmd;

  // Put the help line at the beginning
  cmd.info(help);

  // Train mode
  cmd.addText("\nArguments:");
  cmd.addSCmdArg("file", &file, "the train file");
  cmd.addSCmdArg("model", &model_file, "the model file");

  cmd.addText("\nModel Options:");
  cmd.addRCmdOption("-c", &c_cst, 100., "trade off cst between error/margin");
  cmd.addRCmdOption("-std", &stdv, 12, "the std parameter in the gaussian kernel [exp(-|x-y|^2/std^2)]", true);

  cmd.addRCmdOption("-a", &a_cst, 1., "constant a in the polynomial kernel", true);
  cmd.addRCmdOption("-b", &b_cst, 1., "constant b in the polynomial kernel", true);

  cmd.addText("\nLearning Options:");
  cmd.addRCmdOption("-e", &accuracy, 0.01, "end accuracy");
  cmd.addRCmdOption("-m", &cache_size, 50., "cache size in Mo");
  cmd.addICmdOption("-h", &iter_shrink, 100, "minimal number of iterations before shrinking");

  cmd.addText("\nMisc Options:");
  cmd.addSCmdOption("-dir", &dir_name, ".", "directory to save measures");
  cmd.addBCmdOption("-bin", &binary_mode, false, "binary mode for files");

  // KFold mode (one difference with previous mode: no model is available)
  cmd.addMasterSwitch("--kfold");
  cmd.addText("\nArguments:");
  cmd.addSCmdArg("file", &file, "the train file");
  cmd.addICmdArg("k", &k_fold, "number of folds");

  cmd.addText("\nModel Options:");
  cmd.addRCmdOption("-c", &c_cst, 100., "trade off cst between error/margin");
  cmd.addRCmdOption("-std", &stdv, 12, "the std parameter in the gaussian kernel", true);

  cmd.addRCmdOption("-a", &a_cst, 1., "constant a in the polynomial kernel", true);
  cmd.addRCmdOption("-b", &b_cst, 1., "constant b in the polynomial kernel", true);

  cmd.addText("\nLearning Options:");
  cmd.addRCmdOption("-e", &accuracy, 0.01, "end accuracy");
  cmd.addRCmdOption("-m", &cache_size, 50., "cache size in Mo");
  cmd.addICmdOption("-h", &iter_shrink, 100, "minimal number of iterations before shrinking");

  cmd.addText("\nMisc Options:");
  cmd.addSCmdOption("-dir", &dir_name, ".", "directory to save measures");
  cmd.addBCmdOption("-bin", &binary_mode, false, "binary mode for files");

  // Test mode
  cmd.addMasterSwitch("--test");
  cmd.addText("\nArguments:");
  cmd.addSCmdArg("model", &model_file, "the model file");
  cmd.addSCmdArg("file", &file, "the test file");

  cmd.addText("\nMisc Options:");
  cmd.addSCmdOption("-dir", &dir_name, ".", "directory to save measures");
  cmd.addBCmdOption("-bin", &binary_mode, false, "binary mode for files");

  // Read the command line
  int mode = cmd.read(argc, argv);

  DiskXFile *model = NULL;
  if(mode == 2)
    model = new(allocator) DiskXFile(model_file, "r");

  if(mode < 2)
      Random::seed();
  cmd.setWorkingDirectory(dir_name);

  //=================== Create the SVM... =========================
  Kernel *kernel = new(allocator) GaussianKernel(1./(stdv*stdv));
  SVM *svm = new(allocator) SVMClassification(kernel);

  if(mode < 2)
  {
    svm->setROption("C", c_cst);
    svm->setROption("cache size", cache_size);
  }
  //=================== DataSets & Measurers... ===================

  // Create the training dataset
  MatDataSet *mat_data = new(allocator) MatDataSet(file, -1, 1, false, -1, binary_mode);
  MatDataSet *orig_mat_data = new(allocator) MatDataSet(file, -1, 1, false, -1, binary_mode);
  Sequence *class_labels = new(allocator) Sequence(2, 1);
  class_labels->frames[0][0] = -1;
  class_labels->frames[0][1] = 1;

  MeanVarNorm *mv_norm = new(allocator) MeanVarNorm(mat_data);
  if(mode == 2)
      mv_norm->loadXFile(model);
  mat_data->preProcess(mv_norm);

  DataSet *data = new(allocator) ClassFormatDataSet(mat_data, class_labels);
  DataSet *orig_data = new(allocator) ClassFormatDataSet(orig_mat_data, class_labels);

  // The list of measurers...
  MeasurerList measurers;
  TwoClassFormat *class_format = new(allocator) TwoClassFormat(data);
  ClassMeasurer *class_meas = new(allocator) ClassMeasurer(svm->outputs, data, class_format, cmd.getXFile("the_class_err"));
  if(mode > 0)
      measurers.addNode(class_meas);

  // Reload the model in test mode
  if(mode == 2)
    svm->loadXFile(model);
  
  //=================== The Trainer ===============================

  QCTrainer trainer(svm);
  if(mode == 0)
  {
    trainer.setROption("end accuracy", accuracy);
    trainer.setIOption("iter shrink", iter_shrink);
  }

  //=================== Let's go... ===============================

  // Train
  if(mode == 0)
  {
    trainer.train(data, NULL);
    message("%d SV with %d at bounds", svm->n_support_vectors, svm->n_support_vectors_bound);
    DiskXFile model_(model_file, "w");
    mv_norm->saveXFile(&model_);
    svm->saveXFile(&model_);
  }

  // KFold
  if(mode == 1)
  {
    KFold k(&trainer, k_fold);
    k.crossValidate(data, NULL, &measurers);
  }

  // Test
  if(mode == 2) {
    trainer.test(&measurers);

    SVMSimilarityModel model;

    float correct = 0, wrong = 0;
    for (int t = 0; t < data->n_examples; t++) {
        data->setExample(t);
        orig_data->setExample(t);

        svm->forward(data->inputs);
        int true_class = class_format->getClass(data->targets->frames[0]);
        int obs_class = class_format->getClass(svm->outputs->frames[0]);
        (true_class == obs_class ? correct : wrong) += 1;
        float score = svm->outputs->frames[0][0] / 3;
        assert(obs_class > 0 || score < 0);
        float model_score = model.evaluate(orig_data->inputs->frames[0]);
        if (fabs(score - model_score) > 0.01)
            cout << "Er: " << score << " vs. " << model_score << endl;
    }

    cout << "TOTAL      : " << correct + wrong << endl;
    cout << "CORRECT    : " << correct << endl;
    cout << "WRONG      : " << wrong << endl;
    cout << "ERROR      : " << wrong / (correct + wrong) << endl;
  }

  delete allocator;
  return(0);
}
