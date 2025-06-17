async function listImages() {
  const res = await fetch('/upload/list.py');
  if (!res.ok) {
    console.error('failed to list images:', res.status);
    return;
  }

  const files = await res.json();
  // filter only the images
  const images = files
    .filter(f => f.type === 'file' 
              && /\.(jpe?g|png|gif|webp)$/i.test(f.name))
    .map(f => f.name);

  const gallery = document.getElementById('gallery');
  gallery.innerHTML = '';
  for (let name of images) {
    const div = document.createElement('div');
    div.className = 'relative group';
    const img = document.createElement('img');
    img.src = window.location.origin + '/upload/' + encodeURIComponent(name);
    img.alt = name;
    const btn = document.createElement('button');
    btn.textContent = '✕';
    btn.className =
    "absolute top-2 right-2 bg-indigo-600 text-white rounded-full w-8 h-8 flex items-center justify-center shadow-lg hover:bg-red-600 focus:outline-none focus:ring-2 focus:ring-indigo-400 focus:ring-offset-2 transition";
    btn.title = 'Delete';
    btn.onclick = () => deleteImage(name);
    div.append(img, btn);
    gallery.append(div);
  }
}

async function uploadImage(ev) {
  ev.preventDefault();
  const file = ev.target.file.files[0];
  if (!file) return;

  const url = '/upload/';
  const formData = new FormData();
  formData.append('file', file);

  const res = await fetch(url, {
    method: 'POST',
    body: formData
  });

  const msg = document.getElementById('message');
  if (res.status === 201) {
    msg.textContent = 'Uploaded ✅';
    ev.target.reset();
    listImages();
  } else {
    msg.textContent = `Upload failed (${res.status}) ❌`;
  }
}

async function deleteImage(name) {
  if (!confirm(`Delete ${name}?`)) return;
  const res = await fetch(window.location.pathname + '/' + name, {
    method: 'DELETE'
  });
  if (res.status === 204) {
    listImages();
  } else {
    alert('Delete failed');
  }
}

document.getElementById('uploadForm').addEventListener('submit', uploadImage);
window.addEventListener('load', listImages);