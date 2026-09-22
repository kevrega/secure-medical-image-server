import pydicom

from pydicom.dataset import FileDataset, FileMetaDataset
from pydicom.uid import ExplicitVRLittleEndian, MRImageStorage, generate_uid


filename = "data/test.dcm"


# Information needed for the DICOM file itself
file_meta = FileMetaDataset()

file_meta.MediaStorageSOPClassUID = MRImageStorage
file_meta.MediaStorageSOPInstanceUID = generate_uid()
file_meta.TransferSyntaxUID = ExplicitVRLittleEndian
file_meta.ImplementationClassUID = generate_uid()


# Create the DICOM dataset
dataset = FileDataset(
    filename,
    {},
    file_meta=file_meta,
    preamble=b"\0" * 128
)


dataset.SOPClassUID = MRImageStorage
dataset.SOPInstanceUID = file_meta.MediaStorageSOPInstanceUID


# Test patient data
dataset.PatientID = "12345"
dataset.PatientName = "Kevin^Test"
dataset.PatientAge = "024Y"


# Test study data
dataset.Modality = "MR"
dataset.StudyID = "100"
dataset.StudyDescription = "Brain MRI"
dataset.StudyInstanceUID = generate_uid()


# This test DICOM only contains metadata, not image pixels
dataset.save_as(filename)

print("Created data/test.dcm")