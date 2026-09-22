import { useCallback, useEffect, useState } from 'react'
import './App.css'


const API_KEY = 'test123'


async function apiRequest(path, options = {}) {

  const response = await fetch(path, {
    ...options,

    headers: {
      'X-API-Key': API_KEY,
      ...options.headers
    }
  })

  const text = await response.text()

  if (!response.ok) {
    throw new Error(
      text.trim() ||
      `Request failed: ${response.status}`
    )
  }

  return text.trim()
}


function displayText(value) {

  return value
    .replaceAll('^', ' ')
    .replaceAll('_', ' ')
}


function apiToken(value) {

  return value
    .trim()
    .replace(/\s+/g, '_')
}


function parsePatients(text) {

  if (
    !text ||
    text === 'No patients'
  ) {
    return []
  }

  return text
    .split('\n')
    .filter(line =>
      line.trim() !== ''
    )
    .map(line => {

      const parts =
        line.trim().split(/\s+/)

      return {
        id: parts[0],

        name: displayText(
          parts
            .slice(1, -1)
            .join(' ')
        ),

        age:
          parts[
            parts.length - 1
          ]
      }
    })
}


function parseStudies(text) {

  if (
    !text ||
    text === 'No studies'
  ) {
    return []
  }

  return text
    .split('\n')
    .filter(line =>
      line.trim() !== ''
    )
    .map(line => {

      const parts =
        line.trim().split(/\s+/)

      return {
        id: parts[0],

        patientId:
          parts[1],

        description:
          displayText(
            parts
              .slice(2)
              .join(' ')
          )
      }
    })
}


function parseUsers(text) {

  if (
    !text ||
    text === 'No users'
  ) {
    return []
  }

  return text
    .split('\n')
    .filter(line =>
      line.trim() !== ''
    )
    .map(line => {

      const parts =
        line.trim().split(/\s+/)

      return {
        id: parts[0],

        username:
          displayText(
            parts[1]
          ),

        role:
          displayText(
            parts
              .slice(2)
              .join(' ')
          )
      }
    })
}


// Parse the simple image list returned by the backend
function parseImages(text) {

  if (
    !text ||
    text === 'No images'
  ) {
    return []
  }

  return text
    .split('\n')
    .filter(line =>
      line.trim() !== ''
    )
    .map(line => {

      const firstSpace =
        line.indexOf(' ')

      return {
        id:
          line.slice(
            0,
            firstSpace
          ),

        filename:
          line.slice(
            firstSpace + 1
          )
      }
    })
}


function App() {

  const [activePage, setActivePage] =
    useState('overview')

  const [patients, setPatients] =
    useState([])

  const [studies, setStudies] =
    useState([])

  const [users, setUsers] =
    useState([])

  const [serverConnected, setServerConnected] =
    useState(null)

  const [notice, setNotice] =
    useState(null)


  const [patientForm, setPatientForm] =
    useState({
      id: '',
      name: '',
      age: ''
    })

  const [editingPatient, setEditingPatient] =
    useState(false)


  const [studyForm, setStudyForm] =
    useState({
      id: '',
      patientId: '',
      description: ''
    })

  const [editingStudy, setEditingStudy] =
    useState(false)


  const [userForm, setUserForm] =
    useState({
      id: '',
      username: '',
      role: 'doctor'
    })

  const [editingUser, setEditingUser] =
    useState(false)


  const [dicomFile, setDicomFile] =
    useState('test.dcm')

  const [dicomMessage, setDicomMessage] =
    useState('')


  const [selectedStudyId, setSelectedStudyId] =
    useState('')

  const [studyImages, setStudyImages] =
    useState([])

  const [selectedImage, setSelectedImage] =
    useState(null)

  const [analysis, setAnalysis] =
    useState(null)

  const [analyzing, setAnalyzing] =
    useState(false)


  const loadPatients =
    useCallback(
      async () => {

        try {

          const data =
            await apiRequest(
              '/api/patients'
            )

          setPatients(
            parsePatients(data)
          )

          setServerConnected(true)
        }

        catch {
          setServerConnected(false)
        }
      },
      []
    )


  const loadStudies =
    useCallback(
      async () => {

        try {

          const data =
            await apiRequest(
              '/api/studies'
            )

          setStudies(
            parseStudies(data)
          )

          setServerConnected(true)
        }

        catch {
          setServerConnected(false)
        }
      },
      []
    )


  const loadUsers =
    useCallback(
      async () => {

        try {

          const data =
            await apiRequest(
              '/api/users'
            )

          setUsers(
            parseUsers(data)
          )

          setServerConnected(true)
        }

        catch {
          setServerConnected(false)
        }
      },
      []
    )


  // Load images that belong to the currently selected study
  const loadImages =
    useCallback(
      async studyId => {

        if (!studyId) {
          setStudyImages([])
          return
        }


        try {

          const data =
            await apiRequest(
              `/api/images?study_id=${studyId}`
            )

          setStudyImages(
            parseImages(data)
          )
        }

        catch (error) {

          setStudyImages([])

          setNotice({
            type: 'error',
            text: error.message
          })
        }
      },
      []
    )


  const refreshAll =
    useCallback(
      async () => {

        await Promise.all([
          loadPatients(),
          loadStudies(),
          loadUsers()
        ])
      },
      [
        loadPatients,
        loadStudies,
        loadUsers
      ]
    )


  useEffect(() => {
    refreshAll()
  }, [refreshAll])


  useEffect(() => {

    // Refresh the image list whenever another study is selected
    loadImages(
      selectedStudyId
    )

  }, [
    selectedStudyId,
    loadImages
  ])


  async function savePatient(event) {

    event.preventDefault()

    const {
      id,
      name,
      age
    } = patientForm


    if (
      !id ||
      !name.trim() ||
      age === ''
    ) {
      setNotice({
        type: 'error',
        text:
          'Fill in all patient fields.'
      })

      return
    }


    try {

      await apiRequest(
        '/api/patients',
        {
          method:
            editingPatient
              ? 'PUT'
              : 'POST',

          body:
            `${id} ` +
            `${apiToken(name)} ` +
            `${age}`
        }
      )


      setNotice({
        type: 'success',
        text:
          editingPatient
            ? 'Patient updated.'
            : 'Patient added.'
      })


      setPatientForm({
        id: '',
        name: '',
        age: ''
      })

      setEditingPatient(false)

      await loadPatients()
    }

    catch (error) {

      setNotice({
        type: 'error',
        text: error.message
      })
    }
  }


  function editPatient(patient) {

    setPatientForm({
      id: patient.id,
      name: patient.name,
      age: patient.age
    })

    setEditingPatient(true)
  }


  async function deletePatient(patient) {

    if (
      studies.some(
        study =>
          study.patientId ===
          patient.id
      )
    ) {
      setNotice({
        type: 'error',
        text:
          'Delete this patient\'s studies first.'
      })

      return
    }


    if (
      !window.confirm(
        `Delete ${patient.name}?`
      )
    ) {
      return
    }


    try {

      await apiRequest(
        '/api/patients',
        {
          method: 'DELETE',
          body: patient.id
        }
      )

      await loadPatients()
    }

    catch (error) {

      setNotice({
        type: 'error',
        text: error.message
      })
    }
  }


  async function saveStudy(event) {

    event.preventDefault()


    if (
      !studyForm.id ||
      !studyForm.patientId ||
      !studyForm.description.trim()
    ) {
      setNotice({
        type: 'error',
        text:
          'Fill in all study fields.'
      })

      return
    }


    try {

      await apiRequest(
        '/api/studies',
        {
          method:
            editingStudy
              ? 'PUT'
              : 'POST',

          body:
            `${studyForm.id} ` +
            `${studyForm.patientId} ` +
            `${apiToken(
              studyForm.description
            )}`
        }
      )


      setStudyForm({
        id: '',
        patientId: '',
        description: ''
      })

      setEditingStudy(false)

      await loadStudies()
    }

    catch (error) {

      setNotice({
        type: 'error',
        text: error.message
      })
    }
  }


  function editStudy(study) {

    setStudyForm({
      id: study.id,
      patientId:
        study.patientId,
      description:
        study.description
    })

    setEditingStudy(true)
  }


  async function deleteStudy(study) {

    if (
      !window.confirm(
        `Delete study ${study.id}?`
      )
    ) {
      return
    }


    try {

      await apiRequest(
        '/api/studies',
        {
          method: 'DELETE',
          body: study.id
        }
      )

      await loadStudies()
    }

    catch (error) {

      setNotice({
        type: 'error',
        text: error.message
      })
    }
  }


  async function saveUser(event) {

    event.preventDefault()


    if (
      !userForm.id ||
      !userForm.username.trim() ||
      !userForm.role
    ) {
      setNotice({
        type: 'error',
        text:
          'Fill in all user fields.'
      })

      return
    }


    try {

      await apiRequest(
        '/api/users',
        {
          method:
            editingUser
              ? 'PUT'
              : 'POST',

          body:
            `${userForm.id} ` +
            `${apiToken(
              userForm.username
            )} ` +
            `${apiToken(
              userForm.role
            )}`
        }
      )


      setUserForm({
        id: '',
        username: '',
        role: 'doctor'
      })

      setEditingUser(false)

      await loadUsers()
    }

    catch (error) {

      setNotice({
        type: 'error',
        text: error.message
      })
    }
  }


  function editUser(user) {

    setUserForm({
      id: user.id,
      username: user.username,
      role: user.role
    })

    setEditingUser(true)
  }


  async function deleteUser(user) {

    if (
      !window.confirm(
        `Delete ${user.username}?`
      )
    ) {
      return
    }


    try {

      await apiRequest(
        '/api/users',
        {
          method: 'DELETE',
          body: user.id
        }
      )

      await loadUsers()
    }

    catch (error) {

      setNotice({
        type: 'error',
        text: error.message
      })
    }
  }


  async function importDicom() {

    try {

      const result =
        await apiRequest(
          '/api/dicom',
          {
            method: 'POST',
            body:
              dicomFile.trim()
          }
        )

      setDicomMessage(
        result
      )

      await refreshAll()
    }

    catch (error) {

      setDicomMessage(
        error.message
      )
    }
  }


  async function analyzeFile() {

    // An uploaded image must belong to an existing study
    if (!selectedStudyId) {

      setNotice({
        type: 'error',
        text:
          'Choose a study first.'
      })

      return
    }


    if (!selectedImage) {

      setNotice({
        type: 'error',
        text:
          'Choose an image first.'
      })

      return
    }


    if (
      selectedImage.size >
      15 * 1024 * 1024
    ) {
      setNotice({
        type: 'error',
        text:
          'Maximum file size is 15 MB.'
      })

      return
    }


    setAnalyzing(true)
    setAnalysis(null)


    try {

      const response =
        await fetch(
          '/api/analyze-image',
          {
            method: 'POST',

            headers: {
              'X-API-Key':
                API_KEY,

              'X-Filename':
                selectedImage.name,

              // The backend uses this to connect the image to its study
              'X-Study-ID':
                selectedStudyId,

              'Content-Type':
                'application/octet-stream'
            },

            body:
              selectedImage
          }
        )


      const text =
        await response.text()


      if (!response.ok) {
        throw new Error(
          text.trim()
        )
      }


      setAnalysis(
        JSON.parse(text)
      )

      await loadImages(
        selectedStudyId
      )
    }

    catch (error) {

      setAnalysis({
        error:
          error.message.replace(
            /^ERROR:\s*/,
            ''
          )
      })
    }

    finally {
      setAnalyzing(false)
    }
  }


  // Delete an uploaded image from this study
  async function deleteExistingImage(image) {

    if (!selectedStudyId) {
      return
    }


    if (
      !window.confirm(
        `Delete ${image.filename}?`
      )
    ) {
      return
    }


    try {

      await apiRequest(
        '/api/images',
        {
          method: 'DELETE',
          body:
            `${selectedStudyId} ` +
            `${image.id}`
        }
      )


      // Clear an old result that may belong to the deleted image
      setAnalysis(null)

      await loadImages(
        selectedStudyId
      )


      setNotice({
        type: 'success',
        text:
          `${image.filename} deleted.`
      })
    }

    catch (error) {

      setNotice({
        type: 'error',
        text:
          error.message.replace(
            /^ERROR:\s*/,
            ''
          )
      })
    }
  }


  // Analyze an image that is already stored on the server
  async function analyzeExistingImage(image) {

    if (!selectedStudyId) {
      return
    }


    setAnalyzing(true)
    setAnalysis(null)


    try {

      const result =
        await apiRequest(
          '/api/analyze-existing-image',
          {
            method: 'POST',
            body:
              `${selectedStudyId} ` +
              `${image.id}`
          }
        )

      setAnalysis(
        JSON.parse(result)
      )
    }

    catch (error) {

      setAnalysis({
        error:
          error.message.replace(
            /^ERROR:\s*/,
            ''
          )
      })
    }

    finally {
      setAnalyzing(false)
    }
  }


  async function analyzeDemo() {

    setAnalyzing(true)
    setAnalysis(null)


    try {

      const result =
        await apiRequest(
          '/api/analysis'
        )

      setAnalysis(
        JSON.parse(result)
      )
    }

    catch (error) {

      setAnalysis({
        error:
          error.message.replace(
            /^ERROR:\s*/,
            ''
          )
      })
    }

    finally {
      setAnalyzing(false)
    }
  }


  const pages = {
    overview: [
      'Overview',
      'Medical image dashboard'
    ],

    patients: [
      'Patients',
      'Manage patient records'
    ],

    studies: [
      'Studies',
      'Manage medical studies'
    ],

    users: [
      'Users',
      'Manage system users'
    ],

    imaging: [
      'Imaging',
      'Upload and analyse medical images'
    ]
  }


  return (

    <div className="app-shell">

      <aside className="sidebar">

        <div className="brand">

          <div
            className="brand-mark"
            aria-hidden="true"
          />

          <div>
            <strong>
              MedImage
            </strong>

            <span>
              Server
            </span>
          </div>

        </div>


        <nav>

          {Object.keys(pages).map(
            page => (

              <button
                key={page}
                className={
                  activePage === page
                    ? 'nav-item active'
                    : 'nav-item'
                }
                onClick={() =>
                  setActivePage(page)
                }
              >
                {
                  pages[page][0]
                }
              </button>

            )
          )}

        </nav>


        <div className="backend-status">

          <span
            className={
              serverConnected
                ? 'dot online'
                : 'dot offline'
            }
          />

          <div>
            <strong>
              {
                serverConnected
                  ? 'Backend online'
                  : 'Backend offline'
              }
            </strong>

            <small>
              localhost:1337
            </small>
          </div>

        </div>

      </aside>


      <div className="page">

        <header className="topbar">

          <div>
            <h1>
              {
                pages[
                  activePage
                ][0]
              }
            </h1>

            <p>
              {
                pages[
                  activePage
                ][1]
              }
            </p>
          </div>


          <button
            className="secondary-button"
            onClick={refreshAll}
          >
            Refresh data
          </button>

        </header>


        <main className="content">

          {notice && (

            <div
              className={
                `notice ${notice.type}`
              }
            >
              {notice.text}

              <button
                onClick={() =>
                  setNotice(null)
                }
              >
                ×
              </button>
            </div>

          )}


          {activePage === 'overview' && (

            <>

              <div className="stats">

                <div className="stat">
                  <span>
                    Patients
                  </span>

                  <strong>
                    {patients.length}
                  </strong>

                  <small>
                    Stored records
                  </small>
                </div>


                <div className="stat">
                  <span>
                    Studies
                  </span>

                  <strong>
                    {studies.length}
                  </strong>

                  <small>
                    Medical studies
                  </small>
                </div>


                <div className="stat">
                  <span>
                    Users
                  </span>

                  <strong>
                    {users.length}
                  </strong>

                  <small>
                    Registered users
                  </small>
                </div>

              </div>


              <div className="overview-grid">

                <section className="card">

                  <div className="card-title">

                    <div>
                      <h2>
                        Recent patients
                      </h2>

                      <p>
                        Latest patient records.
                      </p>
                    </div>

                    <button
                      className="link-button"
                      onClick={() =>
                        setActivePage(
                          'patients'
                        )
                      }
                    >
                      Manage
                    </button>

                  </div>


                  {patients.length === 0 ? (

                    <div className="empty">
                      No patients yet.
                    </div>

                  ) : (

                    patients
                      .slice(0, 5)
                      .map(patient => (

                        <div
                          className="record-row"
                          key={patient.id}
                        >

                          <div className="avatar">
                            {
                              patient.name
                                .charAt(0)
                                .toUpperCase()
                            }
                          </div>

                          <div className="record-info">
                            <strong>
                              {patient.name}
                            </strong>

                            <span>
                              ID {patient.id}
                            </span>
                          </div>

                          <span>
                            Age {patient.age}
                          </span>

                        </div>

                      ))

                  )}

                </section>


                <section className="card">

                  <div className="card-title">

                    <div>
                      <h2>
                        Recent studies
                      </h2>

                      <p>
                        Latest medical studies.
                      </p>
                    </div>

                    <button
                      className="link-button"
                      onClick={() =>
                        setActivePage(
                          'studies'
                        )
                      }
                    >
                      Manage
                    </button>

                  </div>


                  {studies.length === 0 ? (

                    <div className="empty">
                      No studies yet.
                    </div>

                  ) : (

                    studies
                      .slice(0, 5)
                      .map(study => (

                        <div
                          className="record-row"
                          key={study.id}
                        >

                          <div className="study-icon">
                            S
                          </div>

                          <div className="record-info">
                            <strong>
                              {
                                study.description
                              }
                            </strong>

                            <span>
                              Study {study.id}
                            </span>
                          </div>

                          <span>
                            Patient {
                              study.patientId
                            }
                          </span>

                        </div>

                      ))

                  )}

                </section>

              </div>

            </>

          )}


          {activePage === 'patients' && (

            <>

              <section className="card">

                <h2>
                  {
                    editingPatient
                      ? 'Edit patient'
                      : 'Add patient'
                  }
                </h2>

                <form
                  className="form-grid"
                  onSubmit={savePatient}
                >

                  <label>
                    Patient ID

                    <input
                      type="number"
                      value={
                        patientForm.id
                      }
                      disabled={
                        editingPatient
                      }
                      onChange={event =>
                        setPatientForm({
                          ...patientForm,
                          id:
                            event.target.value
                        })
                      }
                    />
                  </label>


                  <label>
                    Name

                    <input
                      value={
                        patientForm.name
                      }
                      onChange={event =>
                        setPatientForm({
                          ...patientForm,
                          name:
                            event.target.value
                        })
                      }
                    />
                  </label>


                  <label>
                    Age

                    <input
                      type="number"
                      value={
                        patientForm.age
                      }
                      onChange={event =>
                        setPatientForm({
                          ...patientForm,
                          age:
                            event.target.value
                        })
                      }
                    />
                  </label>


                  <div className="form-buttons">

                    <button
                      className="primary-button"
                    >
                      {
                        editingPatient
                          ? 'Save changes'
                          : 'Add patient'
                      }
                    </button>


                    {editingPatient && (

                      <button
                        type="button"
                        className="secondary-button"
                        onClick={() => {
                          setEditingPatient(false)

                          setPatientForm({
                            id: '',
                            name: '',
                            age: ''
                          })
                        }}
                      >
                        Cancel
                      </button>

                    )}

                  </div>

                </form>

              </section>


              <DataTable
                type="patient"
                rows={patients}
                onEdit={editPatient}
                onDelete={deletePatient}
              />

            </>

          )}


          {activePage === 'studies' && (

            <>

              <section className="card">

                <h2>
                  {
                    editingStudy
                      ? 'Edit study'
                      : 'Add study'
                  }
                </h2>


                <form
                  className="form-grid"
                  onSubmit={saveStudy}
                >

                  <label>
                    Study ID

                    <input
                      type="number"
                      value={
                        studyForm.id
                      }
                      disabled={
                        editingStudy
                      }
                      onChange={event =>
                        setStudyForm({
                          ...studyForm,
                          id:
                            event.target.value
                        })
                      }
                    />
                  </label>


                  <label>
                    Patient

                    <select
                      value={
                        studyForm.patientId
                      }
                      onChange={event =>
                        setStudyForm({
                          ...studyForm,
                          patientId:
                            event.target.value
                        })
                      }
                    >

                      <option value="">
                        Select patient
                      </option>

                      {patients.map(
                        patient => (

                          <option
                            key={
                              patient.id
                            }
                            value={
                              patient.id
                            }
                          >
                            {
                              patient.id
                            }
                            {' - '}
                            {
                              patient.name
                            }
                          </option>

                        )
                      )}

                    </select>

                  </label>


                  <label>
                    Description

                    <input
                      value={
                        studyForm.description
                      }
                      onChange={event =>
                        setStudyForm({
                          ...studyForm,
                          description:
                            event.target.value
                        })
                      }
                    />
                  </label>


                  <div className="form-buttons">

                    <button
                      className="primary-button"
                    >
                      {
                        editingStudy
                          ? 'Save changes'
                          : 'Add study'
                      }
                    </button>


                    {editingStudy && (

                      <button
                        type="button"
                        className="secondary-button"
                        onClick={() => {
                          setEditingStudy(false)

                          setStudyForm({
                            id: '',
                            patientId: '',
                            description: ''
                          })
                        }}
                      >
                        Cancel
                      </button>

                    )}

                  </div>

                </form>

              </section>


              <DataTable
                type="study"
                rows={studies}
                onEdit={editStudy}
                onDelete={deleteStudy}
              />

            </>

          )}


          {activePage === 'users' && (

            <>

              <section className="card">

                <h2>
                  {
                    editingUser
                      ? 'Edit user'
                      : 'Add user'
                  }
                </h2>


                <form
                  className="form-grid"
                  onSubmit={saveUser}
                >

                  <label>
                    User ID

                    <input
                      type="number"
                      value={
                        userForm.id
                      }
                      disabled={
                        editingUser
                      }
                      onChange={event =>
                        setUserForm({
                          ...userForm,
                          id:
                            event.target.value
                        })
                      }
                    />
                  </label>


                  <label>
                    Username

                    <input
                      value={
                        userForm.username
                      }
                      onChange={event =>
                        setUserForm({
                          ...userForm,
                          username:
                            event.target.value
                        })
                      }
                    />
                  </label>


                  <label>
                    Role

                    <select
                      value={
                        userForm.role
                      }
                      onChange={event =>
                        setUserForm({
                          ...userForm,
                          role:
                            event.target.value
                        })
                      }
                    >
                      <option value="doctor">
                        Doctor
                      </option>

                      <option value="radiologist">
                        Radiologist
                      </option>

                      <option value="technician">
                        Technician
                      </option>

                      <option value="admin">
                        Admin
                      </option>

                      <option value="viewer">
                        Viewer
                      </option>
                    </select>
                  </label>


                  <div className="form-buttons">

                    <button
                      className="primary-button"
                    >
                      {
                        editingUser
                          ? 'Save changes'
                          : 'Add user'
                      }
                    </button>


                    {editingUser && (

                      <button
                        type="button"
                        className="secondary-button"
                        onClick={() => {
                          setEditingUser(false)

                          setUserForm({
                            id: '',
                            username: '',
                            role: 'doctor'
                          })
                        }}
                      >
                        Cancel
                      </button>

                    )}

                  </div>

                </form>

              </section>


              <DataTable
                type="user"
                rows={users}
                onEdit={editUser}
                onDelete={deleteUser}
              />

            </>

          )}


          {activePage === 'imaging' && (

            <>

              <div className="imaging-top">

                <section className="card">

                  <h2>
                    Upload image
                  </h2>

                  <p className="muted">
                    Choose a study, upload a DICOM,
                    PNG or JPG and run K-Means segmentation.
                  </p>


                  <label>
                    Study

                    <select
                      value={
                        selectedStudyId
                      }
                      onChange={event => {
                        setSelectedStudyId(
                          event.target.value
                        )

                        setAnalysis(null)
                      }}
                    >

                      <option value="">
                        Select study
                      </option>

                      {studies.map(
                        study => (

                          <option
                            key={
                              study.id
                            }
                            value={
                              study.id
                            }
                          >
                            {
                              study.id
                            }
                            {' - '}
                            {
                              study.description
                            }
                            {' - '}
                            {
                              patients.find(
                                patient =>
                                  patient.id ===
                                  study.patientId
                              )?.name ||
                              `Patient ${study.patientId}`
                            }
                          </option>

                        )
                      )}

                    </select>

                  </label>


                  {studies.length === 0 && (

                    <div className="file-info">
                      Add or import a study before
                      uploading an image.
                    </div>

                  )}


                  {selectedStudyId && (

                    <div className="study-images">

                      <div className="study-images-title">

                        <strong>
                          Images in this study
                        </strong>

                        <span>
                          {studyImages.length}
                        </span>

                      </div>


                      {studyImages.length === 0 ? (

                        <div className="study-images-empty">
                          No images have been uploaded
                          to this study yet.
                        </div>

                      ) : (

                        <div className="study-image-list">

                          {studyImages.map(
                            image => (

                              <div
                                className="study-image-row"
                                key={image.id}
                              >

                                <div>
                                  <strong>
                                    {image.filename}
                                  </strong>

                                  <span>
                                    Image {image.id}
                                  </span>
                                </div>


                                <div className="study-image-actions">

                                  <button
                                    className="secondary-button"
                                    onClick={() =>
                                      analyzeExistingImage(
                                        image
                                      )
                                    }
                                    disabled={
                                      analyzing
                                    }
                                  >
                                    View / analyze
                                  </button>


                                  <button
                                    className="delete-image-button"
                                    onClick={() =>
                                      deleteExistingImage(
                                        image
                                      )
                                    }
                                    disabled={
                                      analyzing
                                    }
                                  >
                                    Delete
                                  </button>

                                </div>

                              </div>

                            )
                          )}

                        </div>

                      )}

                    </div>

                  )}


                  <label className="file-picker">

                    <input
                      type="file"
                      accept=".dcm,.png,.jpg,.jpeg"
                      onChange={event =>
                        setSelectedImage(
                          event.target
                            .files[0] ||
                          null
                        )
                      }
                    />

                    <span>
                      {
                        selectedImage
                          ? selectedImage.name
                          : 'Choose image'
                      }
                    </span>

                  </label>


                  {selectedImage && (

                    <div className="file-info">
                      {
                        (
                          selectedImage.size /
                          1024
                        ).toFixed(1)
                      }
                      {' KB'}
                    </div>

                  )}


                  <div className="image-buttons">

                    <button
                      className="primary-button"
                      onClick={
                        analyzeFile
                      }
                      disabled={
                        analyzing
                      }
                    >
                      {
                        analyzing
                          ? 'Analyzing...'
                          : 'Upload and analyze'
                      }
                    </button>


                    <button
                      className="secondary-button"
                      onClick={
                        analyzeDemo
                      }
                      disabled={
                        analyzing
                      }
                    >
                      Run demo CT
                    </button>

                  </div>

                </section>


                <section className="card">

                  <h2>
                    Import DICOM metadata
                  </h2>

                  <p className="muted">
                    This uses a DICOM file already
                    stored in the server data folder.
                  </p>


                  <div className="input-row">

                    <input
                      value={dicomFile}
                      onChange={event =>
                        setDicomFile(
                          event.target.value
                        )
                      }
                    />

                    <button
                      className="primary-button"
                      onClick={
                        importDicom
                      }
                    >
                      Import
                    </button>

                  </div>


                  {dicomMessage && (

                    <div className="result-message">
                      {dicomMessage}
                    </div>

                  )}

                </section>

              </div>


              {analysis && (

                <section className="card analysis">

                  <div className="card-title">

                    <div>
                      <h2>
                        Analysis result
                      </h2>

                      <p>
                        Original image and
                        segmented result.
                      </p>
                    </div>

                  </div>


                  {analysis.error ? (

                    <div className="error-box">
                      {analysis.error}
                    </div>

                  ) : (

                    <>

                      <div className="analysis-stats">

                        <div>
                          <span>
                            File
                          </span>

                          <strong>
                            {
                              analysis
                                .source_name
                            }
                          </strong>
                        </div>


                        <div>
                          <span>
                            Type
                          </span>

                          <strong>
                            {
                              analysis
                                .input_type
                            }
                          </strong>
                        </div>


                        <div>
                          <span>
                            Size
                          </span>

                          <strong>
                            {
                              analysis.width
                            }
                            {' × '}
                            {
                              analysis.height
                            }
                          </strong>
                        </div>


                        <div>
                          <span>
                            Modality
                          </span>

                          <strong>
                            {
                              analysis.modality
                            }
                          </strong>
                        </div>

                      </div>


                      <div className="image-results">

                        <ImageCard
                          title="Original image"
                          subtitle="Input"
                          image={
                            analysis
                              .original_image
                          }
                        />


                        <ImageCard
                          title="K-Means result"
                          subtitle="3 pixel clusters"
                          image={
                            analysis
                              .clustered_image
                          }
                        />

                      </div>


                      <div className="cluster-data">

                        <div>
                          <span>
                            Cluster centers
                          </span>

                          <code>
                            {
                              analysis
                                .cluster_centers
                                .map(value =>
                                  Number(value)
                                    .toFixed(1)
                                )
                                .join(', ')
                            }
                          </code>
                        </div>


                        <div>
                          <span>
                            Pixel counts
                          </span>

                          <code>
                            {
                              analysis
                                .counts
                                .join(', ')
                            }
                          </code>
                        </div>

                      </div>

                    </>

                  )}

                </section>

              )}

            </>

          )}

        </main>

      </div>

    </div>

  )
}


function DataTable({
  type,
  rows,
  onEdit,
  onDelete
}) {

  let headers = []

  if (type === 'patient') {
    headers = [
      'ID',
      'Name',
      'Age'
    ]
  }

  if (type === 'study') {
    headers = [
      'Study ID',
      'Patient ID',
      'Description'
    ]
  }

  if (type === 'user') {
    headers = [
      'ID',
      'Username',
      'Role'
    ]
  }


  return (

    <section className="card">

      <h2>
        {
          type === 'patient'
            ? 'Patient records'
            : type === 'study'
              ? 'Studies'
              : 'Users'
        }
      </h2>


      {rows.length === 0 ? (

        <div className="empty">
          No records.
        </div>

      ) : (

        <div className="table-wrap">

          <table>

            <thead>
              <tr>
                {headers.map(
                  header => (
                    <th key={header}>
                      {header}
                    </th>
                  )
                )}

                <th>
                  Actions
                </th>
              </tr>
            </thead>


            <tbody>

              {rows.map(row => (

                <tr key={row.id}>

                  {type === 'patient' && (
                    <>
                      <td>{row.id}</td>
                      <td>
                        <strong>
                          {row.name}
                        </strong>
                      </td>
                      <td>{row.age}</td>
                    </>
                  )}


                  {type === 'study' && (
                    <>
                      <td>{row.id}</td>
                      <td>
                        {row.patientId}
                      </td>
                      <td>
                        <strong>
                          {row.description}
                        </strong>
                      </td>
                    </>
                  )}


                  {type === 'user' && (
                    <>
                      <td>{row.id}</td>
                      <td>
                        <strong>
                          {row.username}
                        </strong>
                      </td>
                      <td>
                        <span className="role">
                          {row.role}
                        </span>
                      </td>
                    </>
                  )}


                  <td>
                    <div className="table-buttons">

                      <button
                        onClick={() =>
                          onEdit(row)
                        }
                      >
                        Edit
                      </button>

                      <button
                        className="delete"
                        onClick={() =>
                          onDelete(row)
                        }
                      >
                        Delete
                      </button>

                    </div>
                  </td>

                </tr>

              ))}

            </tbody>

          </table>

        </div>

      )}

    </section>

  )
}


function ImageCard({
  title,
  subtitle,
  image
}) {

  return (

    <div className="image-card">

      <div className="image-card-title">

        <strong>
          {title}
        </strong>

        <span>
          {subtitle}
        </span>

      </div>


      <div className="medical-image">

        <img
          src={
            'data:image/png;base64,' +
            image
          }
          alt={title}
        />

      </div>

    </div>

  )
}


export default App