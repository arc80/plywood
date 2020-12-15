var highlighted = null;

function highlight(elementID) {
    if (highlighted) {
        highlighted.style.background = "";
    }
    highlighted = document.getElementById(elementID);
    if (highlighted) {
        highlighted.style.background = "#ffffa0";
    }
}

function savePageState() {
    stateData = {
        path: location.pathname,
        pageYOffset: window.pageYOffset
    };
    window.history.replaceState(stateData, null);
}

function navigateTo(path, forward, pageYOffset) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            // Extract page title
            var n = this.responseText.indexOf("\n");
            document.title = this.responseText.substr(0, n);
            document.getElementById("article").innerHTML = this.responseText.substr(n + 1);
            window.scrollTo(0, pageYOffset);

            // Select appropriate TOC entry
            var selected = document.querySelector(".sidebar").getElementsByTagName("li");
            for (var j = 0; j < selected.length; j++) {
                var li = selected[j];
                var mustSelect = (li.parentElement.getAttribute("href") == path);
                li.classList.remove(mustSelect ? "unselected" : "selected");
                li.classList.add(mustSelect ? "selected" : "unselected");
            }

            // Update history
            if (forward) {
                history.pushState(null, null, path);
                savePageState();
            }
        }
    };
    xhttp.open("GET", "/content?path=" + path, true);
    xhttp.send();            
}

function onEndTransition(evt) {
    this.removeEventListener('transitionend', onEndTransition);
    this.style.removeProperty("display");
    this.style.removeProperty("transition");
    this.style.removeProperty("height");
}

window.onload = function() { 
    highlight(location.hash.substr(1));

    var list = document.getElementsByClassName("caret");
    for (var i = 0; i < list.length; i++) {
        list[i].addEventListener("click", function() {
            var childList = this.nextElementSibling || this.parentElement.nextElementSibling;
            if (this.classList.contains("caret-down")) {
                // Collapse
                this.classList.remove("caret-down")
                childList.style.display = "block";
                childList.style.height = childList.scrollHeight + "px";
                childList.classList.remove("active");
                requestAnimationFrame(function() {
                    childList.style.transition = "height 0.15s ease-out";
                    requestAnimationFrame(function() {
                        childList.style.height = "0px";
                        childList.addEventListener('transitionend', onEndTransition);
                    });
                });
            } else {
                // Expand
                this.classList.add("caret-down");
                childList.style.display = "block";
                childList.style.transition = "height 0.15s ease-out";
                childList.style.height = childList.scrollHeight + "px";
                childList.classList.add("active");
                childList.addEventListener('transitionend', onEndTransition);
            }
        });
    }

    var list = document.querySelector(".sidebar").getElementsByTagName("a");
    for (var i = 0; i < list.length; i++) {
        var a = list[i];
        var path = a.getAttribute("href");
        if (path.substr(0, 6) == "/docs/") {
            a.onclick = function() {
                savePageState();
                navigateTo(this.getAttribute("href"), true, 0);
                return false;
            }
        }
    }
}

window.onhashchange = function() { 
    highlight(location.hash.substr(1));
}

window.onscroll = function() {
    savePageState();
}

window.addEventListener("popstate", function(evt) {
    if (evt.state) {
        navigateTo(evt.state.path, false, evt.state.pageYOffset);
    }
});
