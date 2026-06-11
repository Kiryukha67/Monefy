function getAuthToken() {
    const token = localStorage.getItem('token');
    if (!token || token === 'undefined' || token === 'null') return null;
    return token;
}

function clearSession() {
    localStorage.removeItem('token');
    localStorage.removeItem('username');
    localStorage.removeItem('userId');
}

function redirectToAuth() {
    clearSession();
    window.location.replace('auth.html');
}

async function apiFetch(url, options = {}) {
    const token = getAuthToken();
    if (!token) {
        redirectToAuth();
        throw new Error('Unauthorized');
    }

    const headers = new Headers(options.headers || {});
    headers.set('Authorization', `Bearer ${token}`);

    const response = await fetch(url, { ...options, headers });
    if (response.status === 401) {
        redirectToAuth();
        throw new Error('Unauthorized');
    }
    return response;
}
