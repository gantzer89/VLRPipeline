/*
 * VocabDB_test.cpp
 *
 *  Created on: Feb 1, 2014
 *      Author: andresf
 */

#include <gtest/gtest.h>

#include <VocabDB.hpp>

TEST(HierarchicalKMeans, TestDatabase) {

	/////////////////////////////////////////////////////////////////////
	cv::Mat imgDescriptors;
	std::vector<std::string> keysFilenames;
	keysFilenames.push_back("sift_0.bin");
	keysFilenames.push_back("sift_1.bin");
	vlr::Mat data(keysFilenames);
	/////////////////////////////////////////////////////////////////////

	vlr::VocabTreeParams params;
	params["depth"] = 4; // To achieve a 1000 words vocabulary

	cv::Ptr<vlr::VocabTreeReal> tree = new vlr::VocabTreeReal(data, params);

	tree->build();

	tree->save("test_vocab.yaml.gz");

	cv::Ptr<vlr::VocabDB> db = new vlr::HKMDB(false);

	db->loadBoFModel("test_vocab.yaml.gz");

	db->clearDatabase();

	bool gotException = false;
	int imgIdx = 0;
	for (std::string& keyFileName : keysFilenames) {
		try {
			FileUtils::loadDescriptors(keyFileName, imgDescriptors);
			db->addImageToDatabase(imgIdx, imgDescriptors);
		} catch (const std::runtime_error& error) {
			fprintf(stderr, "%s\n", error.what());
			gotException = true;
		}
		++imgIdx;
	}

	// Assert all images where inserted without any problem
	ASSERT_FALSE(gotException);

	cv::Mat dbBoFVector, sumResult;

	// Asserting inverted files are not empty anymore
	for (size_t imgIdx = 0; imgIdx < keysFilenames.size(); ++imgIdx) {
		db->getDatabaseBoFVector(imgIdx, dbBoFVector);
		cv::reduce(dbBoFVector, sumResult, 1, CV_REDUCE_SUM);
		ASSERT_TRUE(sumResult.rows == 1);
		ASSERT_TRUE(sumResult.cols == 1);
		ASSERT_TRUE(sumResult.at<float>(0, 0) != 0);
	}

	db->computeWordsWeights(vlr::TF_IDF);

	db->createDatabase();

	db->normalizeDatabase(vlr::NORM_L1);

	// Asserting DB BoF vectors values are in the range [0,1]
	for (size_t imgIdx = 0; imgIdx < keysFilenames.size(); ++imgIdx) {
		db->getDatabaseBoFVector(imgIdx, dbBoFVector);
		ASSERT_TRUE(dbBoFVector.rows == 1);
		for (int i = 0; i < dbBoFVector.cols; ++i) {
			ASSERT_TRUE(dbBoFVector.at<float>(0, i) >= 0);
			ASSERT_TRUE(dbBoFVector.at<float>(0, i) <= 1);
		}
	}

	db->saveInvertedIndex("test_idf.yaml.gz");

	cv::Ptr<vlr::VocabDB> dbLoad = new vlr::HKMDB(false);

	dbLoad->loadBoFModel("test_vocab.yaml.gz");
	dbLoad->loadInvertedIndex("test_idf.yaml.gz");

	// Assert vocabularies have same size
	ASSERT_TRUE(db->getNumOfWords() == dbLoad->getNumOfWords());

	// Assert inverted indices are equal
	ASSERT_TRUE(*(db->getInvertedIndex()) == *(dbLoad->getInvertedIndex()));

	// Querying the tree using the same documents used for building it,
	// the top result must be the document itself and hence the score must be 1
	imgIdx = 0;

	for (std::string& keyFileName : keysFilenames) {
		cv::Mat scores;

		imgDescriptors = cv::Mat();
		FileUtils::loadDescriptors(keyFileName, imgDescriptors);
		dbLoad->scoreQuery(imgDescriptors, scores, vlr::NORM_L1, vlr::L1);

		// Check that scores has the right type
		EXPECT_TRUE(cv::DataType<float>::type == scores.type());

		// Check that scores is a row vector
		EXPECT_TRUE(1 == scores.rows);

		// Check all DB images have been scored
		EXPECT_TRUE((int )keysFilenames.size() == scores.cols);

		// Sort scores and keep indices
		cv::Mat perm;
		cv::sortIdx(scores, perm, cv::SORT_EVERY_ROW + cv::SORT_DESCENDING);

		// Check all scores have been sorted
		EXPECT_TRUE(scores.rows == perm.rows);
		EXPECT_TRUE(scores.cols == perm.cols);

		EXPECT_TRUE(imgIdx == perm.at<int>(0, 0));
		EXPECT_TRUE(round(scores.at<float>(0, perm.at<int>(0, 0))) == 1.0);
		++imgIdx;
	}

}

TEST(HierarchicalKMajority, TestDatabase) {

	cv::Mat imgDescriptors;

	/////////////////////////////////////////////////////////////////////
	std::vector<std::string> keysFilenames;
	keysFilenames.push_back("brief_0.bin");
	keysFilenames.push_back("brief_1.bin");
	vlr::Mat data(keysFilenames);
	/////////////////////////////////////////////////////////////////////

	vlr::VocabTreeParams params;
	params["depth"] = 3; // To achieve a 1000 words vocabulary

	cv::Ptr<vlr::VocabTreeBin> tree = new vlr::VocabTreeBin(data, params);

	tree->build();

	tree->save("test_vocab.yaml.gz");

	cv::Ptr<vlr::VocabDB> db = new vlr::HKMDB(true);

	db->loadBoFModel("test_vocab.yaml.gz");

	db->clearDatabase();

	bool gotException = false;
	int imgIdx = 0;
	for (std::string& keyFileName : keysFilenames) {
		try {
			FileUtils::loadDescriptors(keyFileName, imgDescriptors);
			db->addImageToDatabase(imgIdx, imgDescriptors);
		} catch (const std::runtime_error& error) {
			fprintf(stderr, "%s\n", error.what());
			gotException = true;
		}
		++imgIdx;
	}

	// Assert all images where inserted without any problem
	ASSERT_FALSE(gotException);

	cv::Mat dbBoFVector, sumResult;

	// Asserting inverted files are not empty anymore
	for (size_t imgIdx = 0; imgIdx < keysFilenames.size(); ++imgIdx) {
		db->getDatabaseBoFVector(imgIdx, dbBoFVector);
		cv::reduce(dbBoFVector, sumResult, 1, CV_REDUCE_SUM);
		ASSERT_TRUE(sumResult.rows == 1);
		ASSERT_TRUE(sumResult.cols == 1);
		ASSERT_TRUE(sumResult.at<float>(0, 0) != 0);
	}

	db->computeWordsWeights(vlr::TF_IDF);

	db->createDatabase();

	db->normalizeDatabase(vlr::NORM_L1);

	// Asserting DB BoF vectors values are in the range [0,1]
	for (size_t imgIdx = 0; imgIdx < keysFilenames.size(); ++imgIdx) {
		db->getDatabaseBoFVector(imgIdx, dbBoFVector);
		ASSERT_TRUE(dbBoFVector.rows == 1);
		for (int i = 0; i < dbBoFVector.cols; ++i) {
			ASSERT_TRUE(dbBoFVector.at<float>(0, i) >= 0);
			ASSERT_TRUE(dbBoFVector.at<float>(0, i) <= 1);
		}
	}

	db->saveInvertedIndex("test_idf.yaml.gz");

	cv::Ptr<vlr::VocabDB> dbLoad = new vlr::HKMDB(true);

	dbLoad->loadBoFModel("test_vocab.yaml.gz");
	dbLoad->loadInvertedIndex("test_idf.yaml.gz");

	// Assert vocabularies have same size
	ASSERT_TRUE(db->getNumOfWords() == dbLoad->getNumOfWords());

	// Assert inverted indices are equal
	ASSERT_TRUE(*(db->getInvertedIndex()) == *(dbLoad->getInvertedIndex()));

	// Querying the tree using the same documents used for building it,
	// the top result must be the document itself and hence the score must be 1
	imgIdx = 0;

	for (std::string& keyFileName : keysFilenames) {
		cv::Mat scores;

		imgDescriptors = cv::Mat();
		FileUtils::loadDescriptors(keyFileName, imgDescriptors);
		dbLoad->scoreQuery(imgDescriptors, scores, vlr::NORM_L1, vlr::L1);

		// Check that scores has the right type
		EXPECT_TRUE(cv::DataType<float>::type == scores.type());

		// Check that scores is a row vector
		EXPECT_TRUE(1 == scores.rows);

		// Check all DB images have been scored
		EXPECT_TRUE((int )keysFilenames.size() == scores.cols);

		// Sort scores and keep indices
		cv::Mat perm;
		cv::sortIdx(scores, perm, cv::SORT_EVERY_ROW + cv::SORT_DESCENDING);

		// Check all scores have been sorted
		EXPECT_TRUE(scores.rows == perm.rows);
		EXPECT_TRUE(scores.cols == perm.cols);

		EXPECT_TRUE(imgIdx == perm.at<int>(0, 0));
		EXPECT_TRUE(round(scores.at<float>(0, perm.at<int>(0, 0))) == 1.0);
		++imgIdx;
	}

}

TEST(ApproximateKMajority, TestDatabase) {

	/////////////////////////////////////////////////////////////////////
	cv::Mat imgDescriptors;
	std::vector<std::string> keysFilenames;
	keysFilenames.push_back("brief_0.bin");
	keysFilenames.push_back("brief_1.bin");
	vlr::Mat data(keysFilenames);
	/////////////////////////////////////////////////////////////////////

	vlr::KMajorityParams params;
	params["num.clusters"] = 1000;
	params["max.iterations"] = 10;

	cv::Ptr<vlr::KMajority> tree = new vlr::KMajority(data, params);

	tree->build();

	tree->save("test_vocab.yaml.gz");

	cv::Ptr<vlr::VocabDB> db = new vlr::AKMajDB();

	db->loadBoFModel("test_vocab.yaml.gz");

	((cv::Ptr<vlr::AKMajDB>) db)->buildNNIndex();
	((cv::Ptr<vlr::AKMajDB>) db)->saveNNIndex("test_nn_index.bin");

	db->clearDatabase();

	bool gotException = false;
	int imgIdx = 0;
	for (std::string& keyFileName : keysFilenames) {
		try {
			FileUtils::loadDescriptors(keyFileName, imgDescriptors);
			db->addImageToDatabase(imgIdx, imgDescriptors);
		} catch (const std::runtime_error& error) {
			fprintf(stderr, "%s\n", error.what());
			gotException = true;
		}
		++imgIdx;
	}

	// Assert all images where inserted without any problem
	ASSERT_FALSE(gotException);

	cv::Mat dbBoFVector, sumResult;

	// Asserting inverted files are not empty anymore
	for (size_t imgIdx = 0; imgIdx < keysFilenames.size(); ++imgIdx) {
		db->getDatabaseBoFVector(imgIdx, dbBoFVector);
		cv::reduce(dbBoFVector, sumResult, 1, CV_REDUCE_SUM);
		ASSERT_TRUE(sumResult.rows == 1);
		ASSERT_TRUE(sumResult.cols == 1);
		ASSERT_TRUE(sumResult.at<float>(0, 0) != 0);
	}

	db->computeWordsWeights(vlr::TF_IDF);

	db->createDatabase();

	db->normalizeDatabase(vlr::NORM_L1);

	// Asserting DB BoF vectors values are in the range [0,1]
	for (size_t imgIdx = 0; imgIdx < keysFilenames.size(); ++imgIdx) {
		db->getDatabaseBoFVector(imgIdx, dbBoFVector);
		ASSERT_TRUE(dbBoFVector.rows == 1);
		for (int i = 0; i < dbBoFVector.cols; ++i) {
			ASSERT_TRUE(dbBoFVector.at<float>(0, i) >= 0);
			ASSERT_TRUE(dbBoFVector.at<float>(0, i) <= 1);
		}
	}

	db->saveInvertedIndex("test_idf.yaml.gz");

	cv::Ptr<vlr::VocabDB> dbLoad = new vlr::AKMajDB();

	dbLoad->loadBoFModel("test_vocab.yaml.gz");

	((cv::Ptr<vlr::AKMajDB>) dbLoad)->loadNNIndex("test_nn_index.bin");

	dbLoad->loadInvertedIndex("test_idf.yaml.gz");

	// Assert vocabularies have same size
	ASSERT_TRUE(db->getNumOfWords() == dbLoad->getNumOfWords());

	// Assert inverted indices are equal
	ASSERT_TRUE(*(db->getInvertedIndex()) == *(dbLoad->getInvertedIndex()));

	// Querying the tree using the same documents used for building it,
	// the top result must be the document itself and hence the score must be 1
	imgIdx = 0;

	for (std::string& keyFileName : keysFilenames) {
		cv::Mat scores;

		imgDescriptors = cv::Mat();
		FileUtils::loadDescriptors(keyFileName, imgDescriptors);
		dbLoad->scoreQuery(imgDescriptors, scores, vlr::NORM_L1, vlr::L1);

		// Check that scores has the right type
		EXPECT_TRUE(cv::DataType<float>::type == scores.type());

		// Check that scores is a row vector
		EXPECT_TRUE(1 == scores.rows);

		// Check all DB images have been scored
		EXPECT_TRUE((int )keysFilenames.size() == scores.cols);

		// Sort scores and keep indices
		cv::Mat perm;
		cv::sortIdx(scores, perm, cv::SORT_EVERY_ROW + cv::SORT_DESCENDING);

		// Check all scores have been sorted
		EXPECT_TRUE(scores.rows == perm.rows);
		EXPECT_TRUE(scores.cols == perm.cols);

		EXPECT_TRUE(imgIdx == perm.at<int>(0, 0));
		EXPECT_TRUE(round(scores.at<float>(0, perm.at<int>(0, 0))) == 1.0);
		++imgIdx;
	}

}

TEST(IncrementalKMeans, TestDatabase) {

	/////////////////////////////////////////////////////////////////////
	cv::Mat imgDescriptors;
	std::vector<std::string> keysFilenames;
	keysFilenames.push_back("brief_0.bin");
	keysFilenames.push_back("brief_1.bin");
	vlr::Mat data(keysFilenames);
	/////////////////////////////////////////////////////////////////////

	vlr::IncrementalKMeansParams params;
	params["num.clusters"] = 1000;

	cv::Ptr<vlr::IncrementalKMeans> tree = new vlr::IncrementalKMeans(data, params);

	tree->build();

	tree->save("test_vocab.yaml.gz");

	cv::Ptr<vlr::VocabDB> db = new vlr::IncrementaKMeansDB();

	db->loadBoFModel("test_vocab.yaml.gz");

	db->clearDatabase();

	bool gotException = false;
	int imgIdx = 0;
	for (std::string& keyFileName : keysFilenames) {
		try {
			FileUtils::loadDescriptors(keyFileName, imgDescriptors);
			db->addImageToDatabase(imgIdx, imgDescriptors);
		} catch (const std::runtime_error& error) {
			fprintf(stderr, "%s\n", error.what());
			gotException = true;
		}
		++imgIdx;
	}

	// Assert all images where inserted without any problem
	ASSERT_FALSE(gotException);

	cv::Mat dbBoFVector, sumResult;

	// Asserting inverted files are not empty anymore
	for (size_t imgIdx = 0; imgIdx < keysFilenames.size(); ++imgIdx) {
		db->getDatabaseBoFVector(imgIdx, dbBoFVector);
		cv::reduce(dbBoFVector, sumResult, 1, CV_REDUCE_SUM);
		ASSERT_TRUE(sumResult.rows == 1);
		ASSERT_TRUE(sumResult.cols == 1);
		ASSERT_TRUE(sumResult.at<float>(0, 0) != 0);
	}

	db->computeWordsWeights(vlr::TF_IDF);

	db->createDatabase();

	db->normalizeDatabase(vlr::NORM_L1);

	// Asserting DB BoF vectors values are in the range [0,1]
	for (size_t imgIdx = 0; imgIdx < keysFilenames.size(); ++imgIdx) {
		db->getDatabaseBoFVector(imgIdx, dbBoFVector);
		ASSERT_TRUE(dbBoFVector.rows == 1);
		for (int i = 0; i < dbBoFVector.cols; ++i) {
			ASSERT_TRUE(dbBoFVector.at<float>(0, i) >= 0);
			ASSERT_TRUE(dbBoFVector.at<float>(0, i) <= 1);
		}
	}

	db->saveInvertedIndex("test_idf.yaml.gz");

	cv::Ptr<vlr::VocabDB> dbLoad = new vlr::IncrementaKMeansDB();

	dbLoad->loadBoFModel("test_vocab.yaml.gz");

	dbLoad->loadInvertedIndex("test_idf.yaml.gz");

	// Assert vocabularies have same size
	ASSERT_TRUE(db->getNumOfWords() == dbLoad->getNumOfWords());

	// Assert inverted indices are equal
	ASSERT_TRUE(*(db->getInvertedIndex()) == *(dbLoad->getInvertedIndex()));

	// Querying the tree using the same documents used for building it,
	// the top result must be the document itself and hence the score must be 1
	imgIdx = 0;

	for (std::string& keyFileName : keysFilenames) {
		cv::Mat scores;

		imgDescriptors = cv::Mat();
		FileUtils::loadDescriptors(keyFileName, imgDescriptors);
		dbLoad->scoreQuery(imgDescriptors, scores, vlr::NORM_L1, vlr::L1);

		// Check that scores has the right type
		EXPECT_TRUE(cv::DataType<float>::type == scores.type());

		// Check that scores is a row vector
		EXPECT_TRUE(1 == scores.rows);

		// Check all DB images have been scored
		EXPECT_TRUE((int )keysFilenames.size() == scores.cols);

		// Sort scores and keep indices
		cv::Mat perm;
		cv::sortIdx(scores, perm, cv::SORT_EVERY_ROW + cv::SORT_DESCENDING);

		// Check all scores have been sorted
		EXPECT_TRUE(scores.rows == perm.rows);
		EXPECT_TRUE(scores.cols == perm.cols);

		EXPECT_TRUE(imgIdx == perm.at<int>(0, 0));
		EXPECT_TRUE(round(scores.at<float>(0, perm.at<int>(0, 0))) == 1.0);
		++imgIdx;
	}

}
