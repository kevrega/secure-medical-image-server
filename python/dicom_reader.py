import pydicom

dataset = pydicom.dcmread("data/test.dcm")
# Load file, get data set

print("Patient ID:", dataset.PatientID)
# Read patient from dataset
print("Patient Name:", dataset.PatientName)
print("Modality:", dataset.Modality)
print("Study Description:", dataset.StudyDescription)