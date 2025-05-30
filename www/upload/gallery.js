async function listImages() {
  // fetch the raw directory listing (autoindex must be ON for /upload/*)
  const res = await fetch(window.location.pathname + '/?autoindex');
  const html = await res.text();
  // parse <a href="…"> entries
  const parser = new DOMParser();
  const doc = parser.parseFromString(html, 'text/html');
  const links = Array.from(doc.querySelectorAll('a'))
                     .map(a => a.getAttribute('href'))
                     .filter(h => /\.(jpe?g|png|gif|webp)$/i.test(h));

  const gallery = document.getElementById('gallery');
  gallery.innerHTML = '';
  for (let href of links) {
    const div = document.createElement('div');
    div.className = 'relative group';
    const img = document.createElement('img');
    img.src = window.location.pathname + '/' + href;
    img.alt = href;
    const btn = document.createElement('button');
    btn.textContent = '✕';
    btn.className =
    "absolute top-2 right-2 bg-indigo-600 text-white rounded-full w-8 h-8 flex items-center justify-center shadow-lg hover:bg-red-600 focus:outline-none focus:ring-2 focus:ring-indigo-400 focus:ring-offset-2 transition";
    btn.title = 'Delete';
    btn.onclick = () => deleteImage(href);
    div.append(img, btn);
    gallery.append(div);
  }
}

async function uploadImage(ev) {
  ev.preventDefault();
  const file = ev.target.file.files[0];
  if (!file) return;

  // ALWAYS PUT to /upload/<filename>
  const url = '/upload/' + encodeURIComponent(file.name);

  const res = await fetch(url, {
    method: 'PUT',
    headers: { 'Content-Type': file.type },
    body: file
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