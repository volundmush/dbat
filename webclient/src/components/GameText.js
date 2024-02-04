import { useSelector } from 'react-redux';

export function GameText() {
    const events = useSelector(state => state.gametext.events);

    console.log(events.length);

    // Create a single HTML string from all events
    const createMarkup = () => {
        return { __html: events.join('') };
    };

    return (
        <div id="gametext" dangerouslySetInnerHTML={createMarkup()} />
    );
}