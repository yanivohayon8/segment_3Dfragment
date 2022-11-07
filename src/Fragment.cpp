#include "Fragment.h"
#include <igl/readOBJ.h>
#include <igl/principal_curvature.h>
#include <igl/adjacency_list.h>
#include <set>
#include <igl/writeOBJ.h>

#include <iostream>
#include <cstdio>
#include <filesystem>

// to remove tmp files

ObjFragment::ObjFragment()
{
}


ObjFragment::ObjFragment(std::string filePath)
{
	m_filePath = filePath;
}

void ObjFragment::load()
{
	Eigen::MatrixXd V;
	igl::readOBJ(m_filePath, V, m_TextureCoordinates, 
		m_Normals, m_Faces, m_Faces2TextureCoordinates, m_Faces2Normals);

	m_Vertices = V.block(0, 0, V.rows(), 3);
	//m_Colors = V.block(0, 3, V.rows(), 3); // when playing the cub file it is seems to be meaningless
	igl::adjacency_list(m_Faces, m_adjacentVertices); //list of lists containing at index i the adjacent vertices of vertex i
	std::vector < std::vector<int>> _;
	igl::vertex_triangle_adjacency(m_Vertices, m_Faces, m_VerticesAdjacentFacesList, _);


	/*Eigen::MatrixXd pd1, pd2;
	Eigen::VectorXd pv1, pv2;
	igl::principal_curvature(m_Vertices, m_Faces, pd1, pd2, pv1, pv2);
	m_MeshCurvedness = 0.5*(pv1.array().square() + pv2.array().square()).sqrt();*/
	
	/*std::ofstream f("..\\fragments\\RPf_00154\\curvedness.txt");
	for (int i = 0 ; i < m_MeshCurvedness.rows(); ++i) {
		f << m_MeshCurvedness(i) << '\n';
	}*/


	m_MeshCurvedness.resize(m_Vertices.rows());
	// read here from file to m_MeshCurvedness
	std::ifstream input("..\\fragments\\RPf_00154\\curvedness.txt");
	int i = 0;
	for (std::string line; getline(input, line); )
	{
		m_MeshCurvedness(i) = std::stod(line);
		++i;
	}
	
}

// Todo : return Segment
//void 
void ObjFragment::extractIntactSurface(std::vector<Eigen::MatrixXd> Vs, std::vector<Eigen::MatrixXd> Fs,Segment &oIntactSurface)
{
	Eigen::VectorXd mesh_curvedness_ = m_MeshCurvedness.array().log(); // calculated by  (0.5 * (pv1_.array().square() + pv2_.array().square()).sqrt()); in the preprocessing
	mesh_curvedness_ = ((mesh_curvedness_.array() - mesh_curvedness_.minCoeff()) / (mesh_curvedness_.maxCoeff() - mesh_curvedness_.minCoeff()));

	std::vector<double> log_norm_curvedness;
	log_norm_curvedness.resize(mesh_curvedness_.size());
	Eigen::VectorXd::Map(&log_norm_curvedness[0], log_norm_curvedness.size()) = mesh_curvedness_;
	std::sort(log_norm_curvedness.begin(), log_norm_curvedness.end()); // Yaniv line
	const auto median_it = log_norm_curvedness.begin() + log_norm_curvedness.size() * 0.65;//0.65 good for fine segmentation  //0.5 good for detecting intact vs fractured when removing region merging 
	std::nth_element(log_norm_curvedness.begin(), median_it, log_norm_curvedness.end());
	//*median_it = 0.76;
	double segment_threshold_value = 0.1;//*median_it; /// Yaniv: threshold of the difference between verticies.

	std::map<int, double> available_curves;
	for (auto i = 0; i < mesh_curvedness_.size(); i++)
	{
		//available_curves[i] = static_cast<float>(mesh_curvedness_[i]);
		available_curves[i] = static_cast<double>(mesh_curvedness_[i]);
	}
	std::vector<std::vector<int>> regions_list_;
	std::vector<std::vector<int>> region_outside_boundary_vertices_list_;

	while (!available_curves.empty())
	{
		std::unordered_map<int, int> current_region;
		std::unordered_map<int, int> current_region_boundary_neighbors;
		std::vector<int> current_seeds;
		current_seeds.reserve(available_curves.size());
		int min_curvature_index;
		//initialize_current_region(available_curves, current_region, current_seeds, min_curvature_index);
		min_curvature_index = available_curves.begin()->first;

		available_curves.erase(min_curvature_index);
		current_region.emplace(min_curvature_index, min_curvature_index); // sort of inserting
		current_seeds.push_back(min_curvature_index);

		grow_current_region(available_curves, current_region, current_region_boundary_neighbors, current_seeds, min_curvature_index, segment_threshold_value);// mani

		std::vector<int> vec_current_region;
		for (const auto& curr : current_region)
			vec_current_region.push_back(curr.second);
		regions_list_.push_back(vec_current_region);

		std::vector<int> vec_current_region_boundary_neighbors;
		for (const auto& curr : current_region_boundary_neighbors)
			vec_current_region_boundary_neighbors.push_back(curr.second);
		region_outside_boundary_vertices_list_.push_back(vec_current_region_boundary_neighbors);
	}
	
	Eigen::MatrixXd piece_vertices_with_region = merge_regions(regions_list_);


		
		int chosenIndex = 0;
		double minMeanCurvedness = 9999999;
		int k = 0;

		for (Segment& seg : segmented_regions_)
		{
			double segCurvedness = 0;
			for (int verIndex : seg.piece_vertices_index_)
			{
				segCurvedness += mesh_curvedness_[verIndex];
			}

			
			double segAvgCur = segCurvedness / seg.piece_vertices_index_.size();

			if (segAvgCur < minMeanCurvedness)
			{
				minMeanCurvedness = segAvgCur;
				chosenIndex = k;
			}

			++k;
		}

		std::set<int> uniqueVerticesIndexes;
		std::set<int> uniqueFacesIndexes;
		for (int vertexIndex : segmented_regions_[chosenIndex].piece_vertices_index_)
		{
			//int  = oIntactSurface.piece_vertices_index_[i];
			//uniqueVerticesIndexes.insert(vertexIndex);
			for (int faceIndex : m_VerticesAdjacentFacesList[vertexIndex])
			{
				uniqueFacesIndexes.insert(faceIndex);

				for (auto j = 0; j < m_Faces.cols(); j++)
				{
					uniqueVerticesIndexes.insert(m_Faces.coeff(faceIndex, j));
				}
			}
		}

		std::vector<int> FragfacesIndexes_;
		std::copy(uniqueFacesIndexes.begin(), uniqueFacesIndexes.end(), std::back_inserter(FragfacesIndexes_));
		std::vector<int>FragVertIndexes_;
		std::copy(uniqueVerticesIndexes.begin(), uniqueVerticesIndexes.end(), std::back_inserter(FragVertIndexes_));

		int nSize= FragVertIndexes_.size();
		oIntactSurface.m_Vertices.resize(nSize, 3);
		std::map<int, int> VertIndex2SegIndex;

		int i = 0;
		for (int vertexIndex : FragVertIndexes_)
		{
			VertIndex2SegIndex.insert({ vertexIndex,i });
			for (int j = 0; j < 3; ++j)
				oIntactSurface.m_Vertices(i, j) = m_Vertices(vertexIndex, j);

			i++;
		}
		
		oIntactSurface.m_Faces.resize(FragfacesIndexes_.size(), 3);
		Eigen::MatrixXi FTC,FN;
		oIntactSurface.m_Faces2Normals.resize(FragfacesIndexes_.size(), 3);
		oIntactSurface.m_Faces2TextureCoordinates.resize(FragfacesIndexes_.size(), 3);

		i = 0;
		for (int ixFragfaces : FragfacesIndexes_)
		{
			for (int j = 0; j < 3; ++j)
			{
				int fragVertIndex = m_Faces(ixFragfaces, j);
				oIntactSurface.m_Faces(i, j) = VertIndex2SegIndex[fragVertIndex];

				fragVertIndex = m_Faces2TextureCoordinates(ixFragfaces, j);
				oIntactSurface.m_Faces2TextureCoordinates(i, j) = fragVertIndex; //VertIndex2SegIndex[fragVertIndex];
				
				fragVertIndex = m_Faces2Normals(ixFragfaces, j);
				oIntactSurface.m_Faces2Normals(i, j) = fragVertIndex; //VertIndex2SegIndex[fragVertIndex];

			}
			++i;
		}

}

void ObjFragment::grow_current_region(std::map<int, double>& available_curves, std::unordered_map<int, int>& current_region, std::unordered_map<int, int>& current_region_boundary_neighbors, std::vector<int> current_seeds, int min_curvature_index, double segment_threshold_value)
{

	auto segment_avg = m_MeshCurvedness(min_curvature_index);
	auto count = 1;
	for (auto i = 0; i < current_seeds.size(); i++)
	{
		for (auto j = 0; j < m_adjacentVertices[current_seeds[i]].size(); j++)
		{
			auto current_neighbor = m_adjacentVertices[current_seeds[i]][j];
			// if ((available_curves.count(current_neighbor) == 1) && (mesh_curvedness_(current_neighbor) * mesh_curvedness_(current_seeds[i]) < segment_threshold_value))
			if ((available_curves.count(current_neighbor) == 1) && (m_MeshCurvedness(current_neighbor) * m_MeshCurvedness(current_seeds[i]) < segment_threshold_value))
				// Yaniv : instead of mult , compare with value under threshold
				//if ((available_curves.count(current_neighbor) == 1) && 1-std::fabs(mesh_curvedness_(current_neighbor) - mesh_curvedness_(current_seeds[i]) < segment_threshold_value))
			{
				current_region.emplace(current_neighbor, current_neighbor);
				available_curves.erase(current_neighbor);
				if (m_MeshCurvedness(current_neighbor) < segment_threshold_value)
				{
					current_seeds.push_back(current_neighbor);
				}
				else
				{
					//add the neighbors to the region boundary
					for (auto k = 0; k < m_adjacentVertices[current_neighbor].size(); k++)
					{
						auto boundary_neighbor = m_adjacentVertices[current_neighbor][k];
						if (current_region.count(boundary_neighbor) == 0)
						{
							current_region_boundary_neighbors.emplace(boundary_neighbor, boundary_neighbor);
						}
					}

				}
			}
			else
			{
				if (current_region.count(current_neighbor) == 0)
				{
					current_region_boundary_neighbors.emplace(current_neighbor, current_neighbor);
				}
			}

		}
	}
}


Eigen::MatrixXd ObjFragment::merge_regions(std::vector<std::vector<int>> regions_list_)
{
	Eigen::MatrixXd vertices_with_region_indication = m_Vertices;
	std::map<int, std::vector<int>> merged_regions_list;
	std::map<int, std::vector<int>> small_regions;
	vertices_with_region_indication.conservativeResize(Eigen::NoChange, m_Vertices.cols() + 1);
	const auto region_index_column = vertices_with_region_indication.cols() - 1;

	double minimal_segment_area_percent = 0.05;

	const auto NO_INPUT = -1;
	for (auto i = 0; i < regions_list_.size(); i++)
	{
		merged_regions_list[i] = regions_list_[i];
		if ((static_cast<double>(regions_list_[i].size()) / static_cast<double>(m_Vertices.rows())) <= minimal_segment_area_percent)
		{
			small_regions[i] = regions_list_[i];
		}
		else
		{
			const Segment current = Segment(regions_list_[i]);
			segmented_regions_.push_back(current);
		}

		////if I want to color all formed regions
		 //const segment current = segment(regions_list_[i], *this);
		 //segmented_regions_.insert({ i,current });

		for (auto j = 0; j < regions_list_[i].size(); j++)
		{
			vertices_with_region_indication(regions_list_[i][j], region_index_column) = i;
		}

	}

	////if I want to color all big regions and all small regions as one
	// segment all_small_segments = segment(small_regions[0], *this);
	// for (auto i = 1; i < small_regions.size(); i++)
	// {
	// 	all_small_segments.piece_vertices_index_.insert(all_small_segments.piece_vertices_index_.end(), small_regions[i].begin(), small_regions[i].end());
	// }
	// segmented_regions_.insert({ segmented_regions_.size(), all_small_segments });


	Eigen::MatrixXd updated_vertices_with_region_indication = vertices_with_region_indication;
	return updated_vertices_with_region_indication;
}


void ObjFragment::saveAsObj(std::string outputPath, Segment &iSegment)
{
	std::string tmpFilePath = m_filePath + ".tmp"; //"..\\fragments\\cube\\cube_igl_tmp.obj";
	igl::writeOBJ(tmpFilePath, iSegment.m_Vertices, iSegment.m_Faces, m_Normals,
		iSegment.m_Faces2Normals, m_TextureCoordinates, iSegment.m_Faces2TextureCoordinates);

	std::ifstream input(m_filePath);
	std::string mtlibLine;
	std::string materialLine;

	for (std::string line; getline(input, line); )
	{
		if (line.find("mtllib") != std::string::npos)
		{
			mtlibLine = line;
		}

		if (line.find("usemtl") != std::string::npos)
		{
			materialLine = line;
		}

		if (mtlibLine.length() > 0 && materialLine.length() > 0)
		{
			break;
		}

	}

	std::ofstream outFile(outputPath);
	outFile << "# Generated with Libigl \n" << mtlibLine << '\n' << materialLine << "\n\n";

	{
		std::ifstream objData(tmpFilePath);

		for (std::string line; getline(objData, line); )
		{
			outFile << line << "\n";
		}
	}

	std::filesystem::remove(tmpFilePath);
}
