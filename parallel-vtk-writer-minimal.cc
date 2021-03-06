#include <sstream>
#include <vector>

#include <omp.h>

#include <vtkCPDataDescription.h>
#include <vtkDoubleArray.h>
#include <vtkDoubleArray.h>
#include <vtkImageData.h>
#include <vtkIntArray.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiPieceDataSet.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLPMultiBlockDataWriter.h>
#include <vtkXMLPUnstructuredGridWriter.h>
#include <vtkXMLUnstructuredGridWriter.h>

void Write(int max_threads, std::vector<vtkUnstructuredGrid*> grids) {
  // Write pvtu file using the dummy grid.
  // This will create the correct pvtu file, but incorrect vtu files,
  // because the dummy grid only contains one data element.
  vtkNew<vtkXMLPUnstructuredGridWriter> pvtu_writer;
  pvtu_writer->SetFileName("Cells.pvtu");
  pvtu_writer->SetNumberOfPieces(max_threads);
  pvtu_writer->SetStartPiece(0);
  pvtu_writer->SetEndPiece(max_threads - 1);
  pvtu_writer->SetInputData(grids[max_threads]);
  pvtu_writer->Write();

// Here we write the vtu files in parallel.
// This overwrites the incorrect files generated by the
// vtkXMLPUnstructuredGridWriter.
#pragma omp parallel for schedule(static, 1)
  for (int i = 0; i < max_threads; ++i) {
    vtkNew<vtkXMLUnstructuredGridWriter> vtu_writer;
    std::stringstream s;
    s << "Cells_" << i << ".vtu";
    vtu_writer->SetFileName(s.str().c_str());
    vtu_writer->SetInputData(grids[i]);
    vtu_writer->Write();
  }
}

int main() {
  int max_threads = omp_get_max_threads();

  // construct
  std::vector<vtkUnstructuredGrid*> grids;
  //   create one more grid that will be used to generate the pvtu file
  //   It must have the same structure (data arrays) as the real grids,
  //   and one element per data array.
  grids.resize(max_threads + 1);
  for (int i = 0; i <= max_threads; ++i) {
    grids[i] = vtkUnstructuredGrid::New();
  }

  // add data arrays
  for (int i = 0; i <= max_threads; ++i) {
    // position array
    {
      vtkNew<vtkDoubleArray> new_vtk_array;
      new_vtk_array->SetName("position");
      auto* vtk_array = new_vtk_array.GetPointer();
      vtk_array->SetNumberOfComponents(3);
      vtk_array->InsertNextTuple3(0, 0, 20 * i);
      vtkNew<vtkPoints> points;
      points->SetData(vtk_array);
      grids[i]->SetPoints(points.GetPointer());
    }

    // diameter array
    {
      vtkNew<vtkDoubleArray> new_vtk_array;
      new_vtk_array->SetName("diameter");
      auto* vtk_array = new_vtk_array.GetPointer();
      vtk_array->SetNumberOfComponents(1);
      vtk_array->InsertNextTuple1(5 * ((i % 5) + 1));
      auto* point_data = grids[i]->GetPointData();
      point_data->AddArray(vtk_array);
    }
  }

  // write
  Write(max_threads, grids);

  // destruct
  for (int i = 0; i <= max_threads; ++i) {
    grids[i]->Delete();
  }

  return 0;
}
