export default function NotFound() {
  return (
    <div style={{ padding: '4rem', textAlign: 'center', fontFamily: 'sans-serif' }}>
      <h1 style={{ fontSize: '3rem', color: '#00bcd4' }}>404</h1>
      <h2>Page Non Trouvée</h2>
      <p style={{ margin: '1.5rem 0', color: '#666' }}>La page que vous recherchez n'existe pas ou a été déplacée.</p>
      <a href="/OSMV/docs/" style={{ color: '#00bcd4', textDecoration: 'none', fontWeight: 'bold' }}>
        Retourner à la documentation
      </a>
    </div>
  )
}
