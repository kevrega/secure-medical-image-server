import pydicom
from pydicom.dataset import FileDataset, FileMetaDataset
from pydicom.uid import ExplicitVRLittleEndian, MRImageStorage, generate_uid


filename = "data/test.dcm"

file_meta = FileMetaDataset()
file_meta.MediaStorageSOPClassUID = MRImageStorage
file_meta.MediaStorageSOPInstanceUID = generate_uid()
file_meta.TransferSyntaxUID = ExplicitVRLittleEndian
file_meta.ImplementationClassUID = generate_uid()

dataset = FileDataset(
    filename,
    {},
    file_meta=file_meta,
    preamble=b"\0" * 128
)

dataset.SOPClassUID = MRImageStorage
dataset.SOPInstanceUID = file_meta.MediaStorageSOPInstanceUID

dataset.PatientID = "12345"
dataset.PatientName = "Kevin^Test"
dataset.PatientAge = "025Y"

dataset.Modality = "MR"
dataset.StudyID = "100"
dataset.StudyDescription = "Brain MRI"
dataset.StudyInstanceUID = generate_uid()

dataset.save_as(filename)

print("Created data/test.dcm")
